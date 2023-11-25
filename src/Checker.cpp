#include "alert.h"
#include "Checker.h"
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
    case ASTKind::Value:
    case ASTKind::Variable:
      break;

    case ASTKind::Array: {
      for( auto&& x : ast->as<AST::Array>()->elements )
        this->check(x);

      break;
    }

    case ASTKind::CallFunc: {
      auto cf = ast->as<AST::CallFunc>();

      for( auto&& arg : cf->arguments )
        this->check(arg);

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
      for( auto&& x : ast->as<AST::Scope>()->list )
        this->check(x);
        
      break;
    }

    case ASTKind::While: {
      auto x = ast->as<AST::While>();

      this->check(x->cond);
      this->check(x->code);

      break;
    }
  
    default: {
      auto x = ast->as<AST::Expr>();

      this->check(x->left);
      this->check(x->right);

      break;
    }
  }
}

AST::Function const* Checker::findUserDefFunction(std::string_view name) {
  for( auto&& ast : this->_root->list )
    if( ast->kind == ASTKind::Function &&
          ast->as<AST::Function>()->getName() == name )
      return ast->as<AST::Function>();

  return nullptr;
}

} // namespace metro