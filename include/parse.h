#pragma once

#include "ast.h"

namespace metro {

class Parser {

public:

  Parser(Token* token);

  AST::Base* parse();

  AST::Base* factor();
  // AST::Base* mul();
  AST::Base* add();
  AST::Base* expr();

  AST::Base* stmt();



private:

  bool check();
  Token* next();
  
  bool eat(std::string_view);
  Token* expect(std::string_view);

  Token* expectIdentifier();
  AST::Scope* expectScope();

  Token* token;
  Token* ate;
};

} // namespace metro