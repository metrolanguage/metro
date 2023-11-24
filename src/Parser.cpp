#include "alert.h"
#include "Error.h"
#include "GC.h"
#include "Parser.h"

namespace metro {

Parser::Parser(Token* token)
  : token(token),
    ate(nullptr),
    loopCounterDepth(0)
{
}

AST::Base* Parser::parse() {
  auto ast = new AST::Scope(nullptr);

  while( this->check() ) {
    if( this->eat("def") ) {
      auto func = new AST::Function(this->ate);

      func->name_token = this->expectIdentifier();
      
      this->expect("(");

      if( !this->eat(")") ) {
        do {
        } while( this->eat(",") );

        this->expect(")");
      }

      func->scope = this->expectScope();

      ast->statements.emplace_back(func);

      continue;
    }

    ast->statements.emplace_back(this->stmt());
  }

  return ast;
}

AST::Base* Parser::factor() {
  auto tok = this->token;

  switch( tok->kind ) {
    case TokenKind::Int: {
      this->next();
      return new AST::Value(tok,
        new objects::Int(std::stoi(std::string(tok->str))));
    }

    case TokenKind::String: {
      this->next();
      return new AST::Value(tok,
        new objects::String(std::string(tok->str)));
    }

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

AST::Base* Parser::indexref() {


  todo_impl; 
}

AST::Base* Parser::add() {
  auto x = this->factor();

  while( this->check() ) {
    if( this->eat("+") )
      x = new AST::Expr(ASTKind::Add, this->ate, x, this->factor());
    else
      break;
  }

  return x;
}

AST::Base* Parser::expr() {
  return this->add();
}

AST::Base* Parser::stmt() {

  if( this->token->str == "{" ) {
    return this->expectScope();
  }

  if( this->eat("if") ) {
    auto ast = new AST::If(this->ate);

    ast->cond = this->expr();
    ast->case_true = this->stmt();

    if( this->eat("else") ) {
      ast->case_false = this->stmt();
    }

    return ast;
  }

  if( this->eat("while") ) {
    auto ast = new AST::While(this->ate);

    ast->cond = this->expr();
    ast->code = this->stmt();

    return ast;
  }

  /*
   * make for-loop:
   * create a new scope and use while-statement.
   *
   * 
   */
  if( this->eat("for") ) {
    auto scope = new AST::Scope(this->ate);

    // make loop counter
    auto counterName = "@count" + std::to_string(this->loopCounterDepth);

    auto& assign = scope->statements.emplace_back(
        new AST::Expr(ASTKind::Assignment, nullptr,
            new AST::Variable(ASTKind::Variable, counterName),
            new AST::Value(nullptr, new objects::Int(0))));

    auto ast = new AST::While(this->ate);

    auto iter = this->expr();

    // todo

    alert;
    exit(99);
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
    ast->statements.emplace_back(this->stmt());

  if( !closed ) {
    Error(ast->token)
      .setMessage("scope never closed")
      .emit()
      .exit();
  }

  return ast;
}

} // namespace metro