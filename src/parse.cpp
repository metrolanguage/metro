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

void Parser::expect(std::string_view s) {
  if( !this->eat(s) )
    Error(this->token)
      .setMessage("expect")
      .emit()
      .exit();
}

} // namespace metro