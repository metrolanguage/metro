#pragma once

#include "ast.h"

namespace metro {

class Checker {
public:

  Checker(AST::Scope* root);

  void check(AST::Base* ast);

private:

  //
  // find user-defined function
  AST::Function* findUserDefFunction(std::string_view name);

  AST::Scope* _root;
};

} // namesapce metro