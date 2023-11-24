#include <iostream>
#include <sstream>
#include "alert.h"
#include "eval.h"
#include "gc.h"
#include "BuiltinFunc.h"

namespace metro {

using namespace objects;

Object* Evaluator::eval(AST::Base* ast) {

  switch( ast->kind ) {
    case ASTKind::Function:
      return None::getNone();

    case ASTKind::Value: {
      return ast->as<AST::Value>()->object;
    }

    case ASTKind::Variable: {
      return
        this->getCurrentStorage()[ast->as<AST::Variable>()->getName()];
    }

    case ASTKind::CallFunc: {
      auto cf = ast->as<AST::CallFunc>();

      std::vector<Object*> args;

      for( auto&& arg : cf->arguments )
        args.emplace_back(this->eval(arg));

      if( cf->userdef ) {
        auto& stack = this->push_stack(cf->userdef);

        this->eval(cf->userdef->scope);

        auto result = stack.result;

        this->pop_stack();

        return result;
      }

      return cf->builtin->call(std::move(args));
    }

    case ASTKind::Assignment: {
      auto assign = ast->as<AST::Expr>();

      auto value = this->eval(assign->right);

      if( assign->left->kind == ASTKind::Variable ) {
        auto& storage = this->getCurrentStorage();
        auto name = assign->left->as<AST::Variable>()->getName();

        if( !storage.contains(name) ) {
          gc::bind(storage[name] = value);
          return value;
        }
        else
          gc::unbind(storage[name]);
      }

      return this->evalAsLeft(assign->left) = value;
    }

    case ASTKind::Scope: {
      auto scope = ast->as<AST::Scope>();

      for( auto&& x : scope->statements )
        this->eval(x);

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

Object*& Evaluator::evalAsLeft(AST::Base* ast) {
  switch( ast->kind ) {
    case ASTKind::Variable: {
      return this->getCurrentStorage()[ast->as<AST::Variable>()->getName()];
    }
  }

  alert;
  std::exit(111);
}

Evaluator::CallStack& Evaluator::push_stack(AST::Function const* func) {
  auto& stack = this->callStacks.emplace_back(func);

  return stack;
}

void Evaluator::pop_stack() {
  auto& stack = this->getCurrentCallStack();

  for( auto&& [_, lvar] : stack.storage ) {
    gc::unbind(lvar);
  }

  this->callStacks.pop_back();
}

} // namespace metro