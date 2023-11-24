#include "parse.h"
#include "Error.h"
#include "gc.h"

namespace metro {

Parser::Parser(Token* token)
  : token(token),
    ate(nullptr)
{
}

AST::Base* Parser::parse() {
  auto ast = new AST::Scope();

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
        gc::newObject<objects::Int>(std::stoi(std::string(tok->str))));
    }

    case TokenKind::String: {
      this->next();
      return new AST::Value(tok,
        gc::newObject<objects::String>(std::string(tok->str)));
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
  auto ast = new AST::Scope();

  ast->token = this->expect("{");

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