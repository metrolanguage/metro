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

Evaluator::Evaluator()
  : _loopScope(nullptr),
    _funcScope(nullptr),
    _scope(nullptr)
  {
  }

//
// === eval ===
//
Object* Evaluator::eval(AST::Base* ast) {

  switch( ast->kind ) {
    case ASTKind::Function:
    case ASTKind::Namespace:
      break;

    case ASTKind::Value: {
      return ast->as<AST::Value>()->object;
    }

    case ASTKind::Variable: {
      auto pvar = this->getCurrentStorage()[ast->as<AST::Variable>()->getName()];

      if( !pvar )
        Error(ast)
          .setMessage("variable '" + std::string(ast->token->str) +"' is not defined")
          .emit()
          .exit();

      return pvar->clone();
    }

    case ASTKind::Array: {
      auto obj = new Vector();

      for( auto&& e : ast->as<AST::Array>()->elements )
        obj->append(this->eval(e));
      
      return obj;
    }

    case ASTKind::Tuple: {
      auto obj = new Tuple({ });

      for( auto&& e : ast->as<AST::Array>()->elements )
        obj->elements.emplace_back(this->eval(e));

      return obj;
    }

    case ASTKind::CallFunc: {
      auto cf = ast->as<AST::CallFunc>();

      std::vector<Object*> args;

      for( auto&& arg : cf->arguments )
        args.emplace_back(this->eval(arg));

      if( cf->builtin )
        return cf->builtin->call(std::move(args));

      auto& stack = this->push_stack(cf->userdef);

      for( auto it = args.begin(); auto&& arg : cf->userdef->arguments ) {
        GC::bind(stack.storage[arg->str] = *it++);
      }

      this->eval(cf->userdef->scope);

      auto result = stack.result;

      this->pop_stack();

      if( !result )
        return None::getNone();

      return result;
    }

    case ASTKind::IndexRef: {
      auto x = ast->as<AST::Expr>();

      return this->evalIndexRef(x, this->eval(x->left), this->eval(x->right));
    }

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
            return new USize(obj->as<String>()->value.size());

          break;
        }
      }

      Error(member)
        .setMessage("object of type '" + obj->type.toString()
          + "' don't have a member '" + name + "'")
        .emit()
        .exit();
    }

    case ASTKind::Not: {
      auto x = ast->as<AST::Expr>();

      auto obj = this->eval(x->left);

      if( !obj->type.equals(Type::Bool) ) {
        Error(x->left)
          .setMessage("expected boolean expression")
          .emit()
          .exit();
      }

      obj->as<Bool>()->value ^= 1;
      return obj;
    }

    case ASTKind::LogAND:
    case ASTKind::LogOR: {
      auto x = ast->as<AST::Expr>();

      auto lhs = this->eval(x->left);
      auto rhs = this->eval(x->right);

      if( !lhs->type.equals(Type::Bool) ) {
        Error(x->left)
          .setMessage("expected boolean expression")
          .emit()
          .exit();
      }

      if( !rhs->type.equals(Type::Bool) ) {
        Error(x->right)
          .setMessage("expected boolean expression")
          .emit()
          .exit();
      }

      if( ast->kind == ASTKind::LogAND )
        return new Bool(lhs->as<Bool>()->value && rhs->as<Bool>()->value);

      return new Bool(lhs->as<Bool>()->value || rhs->as<Bool>()->value);
    }

    case ASTKind::Range: {
      auto x = ast->as<AST::Expr>();

      auto begin = this->eval(x->left);
      auto end = this->eval(x->right);

      if( !begin->type.equals(Type::Int) )
        Error(x->left)
          .setMessage("expected 'int' object")
          .emit()
          .exit();

      if( !end->type.equals(Type::Int) )
        Error(x->right)
          .setMessage("expected 'int' object")
          .emit()
          .exit();

      return new Range(begin->as<Int>()->value, end->as<Int>()->value);
    }

    case ASTKind::Assignment: {
      auto assign = ast->as<AST::Expr>();

      auto value = this->eval(assign->right);

      if( assign->left->kind == ASTKind::Variable ) {
        auto& storage = this->getCurrentStorage();
        auto name = assign->left->as<AST::Variable>()->getName();

        if( !storage.contains(name) )
          GC::bind(value);
        else
          GC::unbind(storage[name]);
      }

      return this->evalAsLeft(assign->left) = value;
    }

    case ASTKind::Scope:
    case ASTKind::If:
    case ASTKind::Switch:
    case ASTKind::Return:
    case ASTKind::Break:
    case ASTKind::Continue:
    case ASTKind::Loop:
    case ASTKind::While:
    case ASTKind::DoWhile:
    case ASTKind::For:
      this->evalStatements(ast);
      break;

    default:
      return this->evalOperator(ast->as<AST::Expr>());
  }

  return None::getNone();
}

//
// === evalAsLeft ===
//
Object*& Evaluator::evalAsLeft(AST::Base* ast) {
  switch( ast->kind ) {
    case ASTKind::Variable: {
      return *this->findVariable(ast->as<AST::Variable>()->getName());
    }

    case ASTKind::IndexRef: {
      auto x = ast->as<AST::Expr>();

      return this->evalIndexRef(x, this->evalAsLeft(x->left), this->eval(x->right));
    }

    case ASTKind::MemberAccess:
      return this->evalAsLeft(ast->as<AST::Expr>()->left);
  }

  Error(ast)
    .setMessage("expected writable expression")
    .emit()
    .exit();
}

//
// === evalIndexRef ===
//
Object*& Evaluator::evalIndexRef(AST::Expr* ast, Object* obj, Object* objIndex) {

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
        .setMessage("expected 'int' or 'usize' object")
        .emit()
        .exit();
  }

  switch( obj->type.kind ) {
    case Type::String: {
      return (Object*&)obj->as<String>()->value[index];
    }

    case Type::Vector:
      return obj->as<Vector>()->elements[index];
  }

  Error(ast->right)
    .setMessage("object of type '" + obj->type.toString() + "' is not subscriptable")
    .emit()
    .exit();
}

Evaluator::CallStack& Evaluator::push_stack(AST::Function const* func) {
  return this->callStacks.emplace_back(func);
}

void Evaluator::pop_stack() {
  auto& stack = this->getCurrentCallStack();

  for( auto&& [_, lvar] : stack.storage ) {
    GC::unbind(lvar);
  }

  this->callStacks.pop_back();
}

} // namespace metro