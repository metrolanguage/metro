#pragma once

#include "AST.h"

namespace metro {

class Parser {

public:

  Parser(Token* token);

  AST::Base* parse();

  AST::Base* factor();
  AST::Base* indexref();
  AST::Base* mul();
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

  size_t loopCounterDepth;
};

} // namespace metro