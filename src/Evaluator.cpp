#include <iostream>
#include <sstream>
#include "alert.h"
#include "BuiltinFunc.h"
#include "Evaluator.h"
#include "GC.h"
#include "Utils.h"
#include "Error.h"

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

    case ASTKind::Array: {
      auto obj = new Vector();

      for( auto&& e : ast->as<AST::Array>()->elements )
        obj->append(this->eval(e));
      
      return obj;
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

    case ASTKind::IndexRef:
      return this->evalIndexRef(ast->as<AST::Expr>());

    case ASTKind::MemberAccess: {
      auto x = ast->as<AST::Expr>();

      auto obj = this->eval(x->left);

      auto member = x->right->as<AST::Variable>();
      auto name = member->getName();

      switch( obj->type.kind ) {
        case Type::Int: {
          if( name == "abs" )
            return new Int(std::abs(obj->as<Int>()->value));

          break;
        }

        case Type::String: {
          if( name == "count" )
            return new USize(obj->as<String>()->value.length());

          break;
        }
      }

      Error(member)
        .setMessage("object of type '" + obj->type.to_string()
          + "' don't have a member '" + name + "'")
        .emit()
        .exit();
    }

    case ASTKind::Assignment: {
      auto assign = ast->as<AST::Expr>();

      auto value = this->eval(assign->right);

      if( assign->left->kind == ASTKind::Variable ) {
        auto& storage = this->getCurrentStorage();
        auto name = assign->left->as<AST::Variable>()->getName();

        if( !storage.contains(name) ) {
          GC::bind(storage[name] = value);
          return value;
        }
        else
          GC::unbind(storage[name]);
      }

      return this->evalAsLeft(assign->left) = value;
    }

    case ASTKind::Scope: {
      auto scope = ast->as<AST::Scope>();

      for( auto&& x : scope->list )
        this->eval(x);

      return None::getNone();
    }
  }

  auto expr = ast->as<AST::Expr>();

  auto lhs = this->eval(expr->left)->clone();
  auto rhs = this->eval(expr->right);

  switch( expr->kind ) {
    case ASTKind::Add: {

      lhs->as<Int>()->value += rhs->as<Int>()->value;

      break;
    }

    default:
      todo_impl;
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
    GC::unbind(lvar);
  }

  this->callStacks.pop_back();
}

Object* Evaluator::evalIndexRef(AST::Expr* ast) {

  auto obj = this->eval(ast->left);
  auto objIndex = this->eval(ast->right);

  size_t index = 0;

  switch( objIndex->type.kind ) {
    case Type::Int:
      index = objIndex->as<Int>()->value;
      break;

    case Type::USize:
      index = objIndex->as<USize>()->value;
      break;
    
    default:
      Error(ast->right)
        .setMessage("expected 'usize' object")
        .emit()
        .exit();
  }

  switch( obj->type.kind ) {
    case Type::String: {
      return new Char(obj->as<String>()->value[index]);
    }

    case Type::Vector:
      return obj->as<Vector>()->elements[index];
  }

  Error(ast->right)
    .setMessage("object of type '" + obj->type.to_string() + "' is not subscriptable")
    .emit()
    .exit();
}


} // namespace metro