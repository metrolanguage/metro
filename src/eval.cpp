#include "eval.h"

namespace metro {

using namespace objects;

Object* Evaluator::eval(AST::Base* ast) {

  switch( ast->kind ) {
    case ASTKind::Value: {
      return ast->as<AST::Value>()->object;
    }
  }

  auto expr = ast->as<AST::Expr>();

  auto lhs = this->eval(expr->left);
  auto rhs = this->eval(expr->right);

  switch( expr->kind ) {
    case ASTKind::Add: {

      lhs->as<Int>()->value += rhs->as<Int>()->value;

      break;
    }
  }

  return lhs;
}

} // namespace metro