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
    case ASTKind::Value: {
      ast->as<AST::Value>()->object->noDelete = true;
      break;
    }

    case ASTKind::Variable:
      break;

    case ASTKind::Array:
    case ASTKind::Tuple:
      for( auto&& x : ast->as<AST::Array>()->elements )
        this->check(x);

      break;

    case ASTKind::CallFunc: {
      auto cf = ast->as<AST::CallFunc>();

      for( auto&& arg : cf->arguments )
        this->check(arg);

      int fmlCount = 0;
      int actCount = (int)cf->arguments.size();

      if( (cf->userdef = this->findUserDefFunction(cf->getName())) ) {
        fmlCount = cf->userdef->arguments.size();
      }
      else if( (cf->builtin = builtin::BuiltinFunc::find(std::string(cf->getName()))) ) {
        fmlCount = cf->builtin->arg_count;

        if( fmlCount == -1 ) // free args
          break;
      }
      else {
        Error(cf->token)
          .setMessage("undefined function name")
          .emit();
      }

      if( fmlCount < actCount ) {
        Error(cf->token)
          .setMessage("too many arguments")
          .emit();
      }
      else if( fmlCount > actCount ) {
        Error(cf->token)
          .setMessage("too few arguments")
          .emit();
      }

      break;
    }

    case ASTKind::Scope: {
      for( auto&& x : ast->as<AST::Scope>()->list ) {
        this->check(x);
      }

      break;
    }

    case ASTKind::If: {
      auto x = ast->as<AST::If>();

      this->check(x->cond);
      this->check(x->case_true);
      this->check(x->case_false);

      break;
    }

    case ASTKind::While: {
      auto x = ast->as<AST::While>();

      this->check(x->cond);
      this->check(x->code);

      break;
    }

    case ASTKind::For: {
      auto x = ast->as<AST::For>();

      this->check(x->iter);
      this->check(x->content);
      this->check(x->code);

      break;
    }

    case ASTKind::Function: {
      auto x = ast->as<AST::Function>();

      this->check(x->scope);

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