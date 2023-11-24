#include <iostream>
#include <sstream>
#include "eval.h"
#include "gc.h"

namespace metro {

using namespace objects;

Object* Evaluator::eval(AST::Base* ast) {

  switch( ast->kind ) {
    case ASTKind::Value: {
      return ast->as<AST::Value>()->object;
    }

    

    case ASTKind::CallFunc: {
      auto cf = ast->as<AST::CallFunc>();

      std::vector<Object*> args;

      if( cf->getName() == "println") {
        std::stringstream ss;

        for( auto&& arg : args )
          ss << arg->to_string();

        auto s = ss.str();

        std::cout << s << std::endl;

        return gc::newObject<USize>(s.length());
      }

      return None::getNone();
    }
  }

  auto expr = ast->as<AST::Expr>();

  auto lhs = gc::cloneObject(this->eval(expr->left));
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