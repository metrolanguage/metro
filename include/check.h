#pragma once

#include "ast.h"

namespace metro {

class Checker {
public:

  Checker(AST::Base* root);

  void check(AST::Base* ast);

private:

  //
  // find user-defined function
  AST::Function* findUserDefFunction(std::string_view name);

  AST::Base* _root;
};

} // namesapce metro