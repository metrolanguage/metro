#include "check.h"
#include "alert.h"

namespace metro {

Checker::Checker(AST::Base* root)
  : _root(root)
{
}

void Checker::check(AST::Base* ast) {
  if( !ast )
    return;

  switch( ast->kind ) {
    case ASTKind::CallFunc: {

    }
  }
}

AST::Function* Checker::findUserDefFunction(std::string_view name) {
  for( auto&& ast : this->_root->statements )
    if( ast->kind == ASTKind::Function && ast->getName() == name )
      return ast;

  return nullptr;
}

} // namespace metro