#pragma once

#include "AST.h"

namespace metro {

class Parser {
public:
  Parser(Token* token);

  AST::Base* parse();

  AST::Base* factor();
  AST::Base* indexref();
  AST::Base* unary();
  AST::Base* mul();
  AST::Base* add();
  AST::Base* shift();
  AST::Base* range();
  AST::Base* pair();
  AST::Base* compare();
  AST::Base* equality();
  AST::Base* bitAND();
  AST::Base* bitOR();
  AST::Base* bitXOR();
  AST::Base* logAND();
  AST::Base* logOR();
  AST::Base* assign();
  AST::Base* expr();
  AST::Base* stmt();

private:
  bool check();
  Token* next();
  
  bool eat(std::string_view);
  Token* expect(std::string_view);

  Token* expectIdentifier();
  AST::Scope* expectScope();

  AST::Variable* newVariable(std::string const& name);
  AST::Expr* newAssign(AST::Base* dest, AST::Base* src);

  Token* token;
  Token* ate;
};

} // namespace metro