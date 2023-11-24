#include "alert.h"
#include "check.h"
#include "BuiltinFunc.h"
#include "Error.h"

namespace metro {

Checker::Checker(AST::Scope* root)
  : _root(root)
{
}

void Checker::check(AST::Base* ast) {
  if( !ast )
    return;

  switch( ast->kind ) {
    case ASTKind::CallFunc: {
      auto cf = ast->as<AST::CallFunc>();

      if( (cf->userdef = this->findUserDefFunction(cf->getName())) )
        break;

      cf->builtin =
        builtin::BuiltinFunc::find(std::string(cf->getName()));

      if( !cf->builtin )
        Error(cf->token)
          .setMessage("undefined function name")
          .emit();

      break;
    }

    case ASTKind::Scope: {
      for( auto&& x : ast->as<AST::Scope>()->statements )
        this->check(x);
        
      break;
    }
  }
}

AST::Function* Checker::findUserDefFunction(std::string_view name) {
  for( auto&& ast : this->_root->statements )
    if( ast->kind == ASTKind::Function &&
          ast->as<AST::Function>()->getName() == name )
      return ast->as<AST::Function>();

  return nullptr;
}

} // namespace metro