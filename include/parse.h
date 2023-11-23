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


  /*
  todo:

  AST::Base* parseStatement();
  */


private:

  bool check();
  Token* next();
  
  bool eat(std::string_view);
  void expect(std::string_view);

  Token* token;
  Token* ate;
};

} // namespace metro