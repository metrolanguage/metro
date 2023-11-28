#include <cassert>
#include "alert.h"
#include "Error.h"
#include "GC.h"
#include "Lexer.h"
#include "Parser.h"
#include "SourceLoc.h"
#include "Metro.h"

namespace metro {

Parser::Parser(Token* token)
  : token(token),
    ate(nullptr)
{
}

/*
 * parser
 */
AST::Base* Parser::parse() {
  auto ast = new AST::Scope(nullptr);

  while( this->check() ) {
    //
    // import
    //
    if( this->eat("import") ) {
      std::string path;
      AST::Base* read = nullptr;

      do {
        path += this->expectIdentifier()->str;
      } while( this->eat("/") );

      this->expect(";");

      path += ".metro";

      auto mt = Metro::getInstance();
      auto curscript = mt->getRunningScript();

      auto importedScript =
        curscript->_imported.emplace_back(new Metro::ScriptInfo(path));

      auto save = curscript;

      mt->currentScript = importedScript;

      read = Parser(Lexer(importedScript->source).lex()).parse();

      mt->currentScript = save;

      assert(read->kind == ASTKind::Scope);

      for( auto&& e : read->as<AST::Scope>()->list )
        ast->list.emplace_back(e);

      continue;
    }


    //
    // function definition
    //
    if( this->eat("def") ) {
      auto func = new AST::Function(this->ate);

      func->name_token = this->expectIdentifier();
      
      this->expect("(");

      if( !this->eat(")") ) {
        do {
          func->arguments.emplace_back(this->expectIdentifier());
        } while( this->eat(",") );

        this->expect(")");
      }

      func->scope = this->expectScope();

      ast->list.emplace_back(func);

      continue;
    }

    ast->list.emplace_back(this->stmt());
  }

  return ast;
}

/*
 * factor
 */
AST::Base* Parser::factor() {
  auto tok = this->token;

  //
  // brackets
  if( this->eat("(") ) {
    auto x = this->expr();

    //
    // tuple
    if( this->eat(",") ) {
      auto tuple = new AST::Array(tok, { x });

      do {
        tuple->elements.emplace_back(this->expr());
      } while( this->eat(",") );

      x = tuple;
    }

    this->expect(")");
    return x;
  }

  //
  // array
  if( this->eat("[") ) {
    auto ast = new AST::Array(tok);

    if( !this->eat("]") ) {
      do {
        ast->elements.emplace_back(this->expr());
      } while( this->eat(",") );

      this->expect("]");
    }

    return ast;
  }

  //
  // boolean
  //
  if( this->eat("true") )
    return new AST::Value(tok, new objects::Bool(true));

  if( this->eat("false") )
    return new AST::Value(tok, new objects::Bool(false));

  //
  // immediate
  switch( tok->kind ) {
    case TokenKind::Int:
      this->next();
      return new AST::Value(tok, new objects::Int(std::stoll(std::string(tok->str))));

    case TokenKind::Float:
      this->next();
      return new AST::Value(tok, new objects::Float(std::stof(std::string(tok->str))));

    case TokenKind::USize:
      this->next();
      return new AST::Value(tok, new objects::USize(std::stoull(std::string(tok->str))));

    case TokenKind::Char:
      this->next();
      return new AST::Value(tok,
        new objects::Char(objects::String::conv.from_bytes(std::string(tok->str))[0]));

    case TokenKind::String:
      this->next();
      return new AST::Value(tok, new objects::String(std::string(tok->str)));

    case TokenKind::Identifier: {
      this->next();

      if( this->eat("(") ) {
        auto ast = new AST::CallFunc(tok);
        
        if( !this->eat(")") ) {
          do {
            ast->arguments.emplace_back(this->expr());
          } while( this->eat(",") );

          this->expect(")");
        }

        return ast;
      }

      return new AST::Variable(tok);
    }
  }

  Error(this->token)
    .setMessage("invalid syntax")
    .emit()
    .exit();
}

/*
 * indexref
 */
AST::Base* Parser::indexref() {
  auto x = this->factor();

  while( this->check() ) {
    if( this->eat("[") ) {
      x = new AST::Expr(ASTKind::IndexRef, this->ate, x, this->expr());
      this->expect("]");
    }
    else if( this->eat(".") ) {
      x = new AST::Expr(ASTKind::MemberAccess, this->ate, x, this->factor());
    }
    else
      break;
  }

  return x;
}

/*
 * unary
 */
AST::Base* Parser::unary() {
  if( this->eat("-") )
    return new AST::Expr(ASTKind::Sub, this->ate,
      new AST::Value(nullptr, new objects::Int(0)), this->indexref());

  if( this->eat("!") )
    return new AST::Expr(ASTKind::Not, this->ate, this->indexref(), nullptr);

  return this->indexref();
}

/*
 * mul
 */
AST::Base* Parser::mul() {
  auto x = this->unary();

  while( this->check() ) {
    if( this->eat("*") )
      x = new AST::Expr(ASTKind::Mul, this->ate, x, this->unary());
    else if( this->eat("/") )
      x = new AST::Expr(ASTKind::Div, this->ate, x, this->unary());
    else if( this->eat("%") )
      x = new AST::Expr(ASTKind::Mod, this->ate, x, this->unary());
    else
      break;
  }

  return x;
}

/*
 * add
 */
AST::Base* Parser::add() {
  auto x = this->mul();

  while( this->check() ) {
    if( this->eat("+") )
      x = new AST::Expr(ASTKind::Add, this->ate, x, this->mul());
    else if( this->eat("-") )
      x = new AST::Expr(ASTKind::Sub, this->ate, x, this->mul());
    else
      break;
  }

  return x;
}

/*
 * shift
 */
AST::Base* Parser::shift() {
  auto x = this->add();

  while( this->check() ) {
    if( this->eat("<<") )
      x = new AST::Expr(ASTKind::LShift, this->ate, x, this->add());
    else if( this->eat(">>") )
      x = new AST::Expr(ASTKind::RShift, this->ate, x, this->add());
    else
      break;
  }

  return x;
}

/*
 * range
 */
AST::Base* Parser::range() {
  auto x = this->shift();

  if( this->eat("..") )
    return new AST::Expr(ASTKind::Range, this->ate, x, this->shift());

  return x;
}

/*
 * compare
 */
AST::Base* Parser::compare() {
  auto x = this->range();

  while( this->check() ) {
    if( this->eat(">") )
      x = new AST::Expr(ASTKind::Bigger, this->ate, x, this->range());
    else if( this->eat("<") )
      x = new AST::Expr(ASTKind::Bigger, this->ate, this->range(), x);
    else if( this->eat(">=") )
      x = new AST::Expr(ASTKind::BiggerOrEqual, this->ate, x, this->range());
    else if( this->eat("<=") )
      x = new AST::Expr(ASTKind::BiggerOrEqual, this->ate, this->range(), x);
    else
      break;
  }
  
  return x;
}

/*
 * equality
 */
AST::Base* Parser::equality() {
  auto x = this->compare();

  while( this->check() ) {
    if( this->eat("==") )
      x = new AST::Expr(ASTKind::Equal, this->ate, x, this->compare());
    else if( this->eat("!=") )
      x = new AST::Expr(ASTKind::Not, this->ate,
            new AST::Expr(ASTKind::Equal, this->ate, x, this->compare()), nullptr);
    else
      break;
  }
  
  return x;
}

/*
 * bitAND
 */
AST::Base* Parser::bitAND() {
  auto x = this->equality();

  while( this->eat("&") )
    x = new AST::Expr(ASTKind::BitAND, this->ate, x, this->equality());
  
  return x;
}

/*
 * bitOR
 */
AST::Base* Parser::bitOR() {
  auto x = this->bitAND();

  while( this->eat("|") )
    x = new AST::Expr(ASTKind::BitOR, this->ate, x, this->bitAND());
  
  return x;
}

/*
 * bitXOR
 */
AST::Base* Parser::bitXOR() {
  auto x = this->bitOR();

  while( this->eat("^") )
    x = new AST::Expr(ASTKind::BitXOR, this->ate, x, this->bitOR());
  
  return x;
}

/*
 * logAND
 */
AST::Base* Parser::logAND() {
  auto x = this->bitXOR();

  while( this->eat("&&") )
    x = new AST::Expr(ASTKind::LogAND, this->ate, x, this->bitXOR());
  
  return x;
}

/*
 * logOR
 */
AST::Base* Parser::logOR() {
  auto x = this->logAND();

  while( this->eat("||") )
    x = new AST::Expr(ASTKind::LogOR, this->ate, x, this->logAND());
  
  return x;
}

/*
 * assign
 */
AST::Base* Parser::assign() {
  auto x = this->logOR();

  if( this->eat("=") )
    x = new AST::Expr(ASTKind::Assignment, this->ate, x, this->assign());
  
  return x;
}

/*
 * expr
 */
AST::Base* Parser::expr() {
  return this->assign();
}

/*
 * stmt
 */
AST::Base* Parser::stmt() {

  if( this->token->str == "{" ) {
    return this->expectScope();
  }

  //
  // if
  if( this->eat("if") ) {
    auto ast = new AST::If(this->ate);

    ast->cond = this->expr();
    ast->case_true = this->stmt();

    if( this->eat("else") ) {
      ast->case_false = this->stmt();
    }

    return ast;
  }

  //
  // while
  if( this->eat("while") ) {
    auto ast = new AST::While(this->ate);

    ast->cond = this->expr();
    ast->code = this->stmt();

    return ast;
  }

  //
  // for
  if( this->eat("for") ) {
    auto ast = new AST::For(this->ate);

    ast->iter = this->expr();

    this->expect("in");

    ast->content = this->expr();

    ast->code = this->stmt();

    return ast;
  }

  //
  // return
  if( this->eat("return") ) {
    auto x = new AST::Expr(ASTKind::Return, this->ate, nullptr, nullptr);

    if( !this->eat(";") ) {
      x->left = this->expr();
      this->expect(";");
    }

    return x;
  }

  //
  // break
  if( this->eat("break") ) {
    this->expect(";");
    return new AST::Expr(ASTKind::Break, this->ate, nullptr, nullptr);
  }

  //
  // continue
  if( this->eat("continue") ) {
    this->expect(";");
    return new AST::Expr(ASTKind::Continue, this->ate, nullptr, nullptr);
  }

  auto x = this->expr();
  this->expect(";");
  return x;
}

bool Parser::check() {
  return this->token->kind != TokenKind::End;
}

Token* Parser::next() {
  this->token = this->token->next;
  return this->token->prev;
}

bool Parser::eat(std::string_view s) {
  if( this->token->str == s ) {
    this->next();
    return true;
  }

  return false;
}

Token* Parser::expect(std::string_view s) {
  if( !this->eat(s) )
    Error(this->token)
      .setMessage("expected '" + std::string(s) + "'")
      .emit()
      .exit();

  return this->ate;
}

Token* Parser::expectIdentifier() {
  if( this->token->kind != TokenKind::Identifier ) {
    Error(this->token)
      .setMessage("expected identifier")
      .emit()
      .exit();
  }

  this->ate = this->token;
  this->next();

  return this->ate;
}

AST::Scope* Parser::expectScope() {
  auto ast = new AST::Scope(this->expect("{"));

  if( this->eat("}") )
    return ast;

  bool closed = false;

  while( !(closed = this->eat("}")) )
    ast->list.emplace_back(this->stmt());

  if( !closed ) {
    Error(ast->token)
      .setMessage("scope never closed")
      .emit()
      .exit();
  }

  return ast;
}

} // namespace metro