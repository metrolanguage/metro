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

//
// === eval ===
//
Object* Evaluator::eval(AST::Base* ast) {

  switch( ast->kind ) {
    case ASTKind::Function:
      break;

    case ASTKind::Value: {
      return ast->as<AST::Value>()->object;
    }

    case ASTKind::Variable: {
      return
        this->getCurrentStorage()[ast->as<AST::Variable>()->getName()]
            ->clone();
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
        .setMessage("object of type '" + obj->type.to_string()
          + "' don't have a member '" + name + "'")
        .emit()
        .exit();
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

      break;
    }

    case ASTKind::For: {
      auto x = ast->as<AST::For>();

      auto& iter = this->evalAsLeft(x->iter);

      todo_impl;

      break;
    }

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
      return this->getCurrentStorage()[ast->as<AST::Variable>()->getName()];
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
// === evalOperator ===
//
Object* Evaluator::evalOperator(AST::Expr* expr) {
  static void* operator_labels[] = {
    nullptr, // value
    nullptr, // array
    nullptr, // variable
    nullptr, // callfunc
    nullptr, // memberaccess
    nullptr, // indexref

    &&op_add,
    &&op_sub,
    &&op_mul,
    &&op_div,
    &&op_mod,
    &&op_lshift,
    &&op_rshift,
  };

  auto lhs = this->eval(expr->left);
  auto rhs = this->eval(expr->right);

  if( static_cast<int>(expr->kind) >= std::size(operator_labels) )
    todo_impl;

  goto *operator_labels[static_cast<int>(expr->kind)];

  invalidOperator:
    Error(expr->token)
      .setMessage("invalid operator")
      .emit()
      .exit();

op_add:
  switch( lhs->type.kind ) {
    //
    // left is Int
    //
    case Type::Int: {
      switch( rhs->type.kind ) {
        case Type::Int:
          lhs->as<Int>()->value += rhs->as<Int>()->value;
          break;

        case Type::Float:
        in_op_add_int_float:
          lhs->as<Int>()->value += (Int::ValueType)rhs->as<Float>()->value;
          break;

        case Type::USize:
          lhs->as<Int>()->value += (Int::ValueType)rhs->as<USize>()->value;
          break;

        default:
          goto invalidOperator;
      }

      break;
    }

    //
    // left is float
    //
    case Type::Float: {
      switch( rhs->type.kind ) {
        case Type::Int:
          std::swap(lhs, rhs);
          goto in_op_add_int_float;
        
        case Type::Float:
          lhs->as<Float>()->value += rhs->as<Float>()->value;
          break;
        
        case Type::USize:
          lhs->as<Float>()->value += (Float::ValueType)rhs->as<Float>()->value;
          break;

        default:
          goto invalidOperator;
      }

      break;
    }

    case Type::String:


    default:
      goto invalidOperator;
  }
  goto end_label;

op_sub:
  switch( lhs->type.kind ) {
    //
    // left is Int
    //
    case Type::Int: {
      switch( rhs->type.kind ) {
        // int - int
        case Type::Int:
          lhs->as<Int>()->value -= rhs->as<Int>()->value;
          break;

        // int - float
        case Type::Float:
        in_op_sub_int_float:
          lhs->as<Int>()->value -= (Int::ValueType)rhs->as<Float>()->value;
          break;

        // int - usize
        case Type::USize:
          lhs->as<Int>()->value -= (Int::ValueType)rhs->as<USize>()->value;
          break;

        default:
          goto invalidOperator;
      }

      break;
    }

    //
    // left is float
    //
    case Type::Float: {
      switch( rhs->type.kind ) {
        // float - int
        case Type::Int:
          std::swap(lhs, rhs);
          goto in_op_sub_int_float;
        
        // float - float
        case Type::Float:
          lhs->as<Float>()->value -= rhs->as<Float>()->value;
          break;
        
        // float - usize
        case Type::USize:
          lhs->as<Float>()->value -= (Float::ValueType)rhs->as<USize>()->value;
          break;

        default:
          goto invalidOperator;
      }

      break;
    }

    default:
      goto invalidOperator;
  }
  goto end_label;

op_mul:
  switch( lhs->type.kind ) {
    case Type::Int:
      switch( rhs->type.kind ) {
        // int * int
        case Type::Int:
          lhs->as<Int>()->value *= rhs->as<Int>()->value;
          break;

        // int * float
        case Type::Float:
        in_op_mul_int_float:
          lhs->as<Int>()->value *= (Int::ValueType)rhs->as<Float>()->value;
          break;

        // int * usize
        case Type::USize:
        in_op_mul_int_usize:
          lhs->as<Int>()->value *= (Int::ValueType)rhs->as<USize>()->value;
          break;

        // int * string
        case Type::String:
          lhs = new USize(lhs->as<Int>()->value);
          goto in_op_mul_usize_string;

        default:
          goto invalidOperator;
      }
      break;

    case Type::Float:
      switch( rhs->type.kind ) {
        // float * int
        case Type::Int:
          std::swap(lhs, rhs);
          goto in_op_mul_int_float;

        // float * float
        case Type::Float:
          lhs->as<Float>()->value *= rhs->as<Float>()->value;
          break;
        
        // float * usize
        case Type::USize:
        in_op_mul_float_usize:
          lhs->as<Float>()->value *= (Float::ValueType)rhs->as<USize>()->value;
          break;

        default:
          goto invalidOperator;
      }
      break;
    
    case Type::USize:
      switch( rhs->type.kind ) {
        // usize * int
        case Type::Int:
          std::swap(lhs, rhs);
          goto in_op_mul_int_usize;

        // usize * float
        case Type::Float:
          std::swap(lhs, rhs);
          goto in_op_mul_float_usize;

        // usize * usize
        case Type::USize:
          lhs->as<USize>()->value *= rhs->as<USize>()->value;
          break;

        // usize * string
        // => string
        case Type::String: {
        in_op_mul_usize_string:
          auto obj = new String();

          for( USize::ValueType i = 0; i < lhs->as<USize>()->value; i++ )
            for( auto&& c : rhs->as<String>()->value )
              obj->value.emplace_back(c->clone());

          lhs = obj;
          break;
        }

        // usize * vector
        case Type::Vector: {
        in_op_mul_usize_vector:
          auto obj = new Vector();

          for( USize::ValueType i = 0; i < lhs->as<USize>()->value; i++ )
            for( auto&& e : rhs->as<Vector>()->elements )
              obj->elements.emplace_back(e->clone());

          lhs = obj;
          break;
        }

        default:
          goto invalidOperator;
      }
      break;

    case Type::String:
      switch( rhs->type.kind ) {
        // string * int
        case Type::Int:
          rhs = new USize(rhs->as<Int>()->value);
        
        // string * usize
        case Type::USize:
          std::swap(lhs, rhs);
          goto in_op_mul_usize_string;
        
        default:
          goto invalidOperator;
      }
      break;

    case Type::Vector:
      switch( rhs->type.kind ) {
        // vector * int
        case Type::Int:
          rhs = new USize(rhs->as<Int>()->value);

        // vector * usize
        case Type::USize:
          std::swap(lhs, rhs);
          goto in_op_mul_usize_vector;

        default:
          goto invalidOperator;
      }
      break;

    default:
      goto invalidOperator;
  }
  goto end_label;

op_div:
  switch( lhs->type.kind ) {
    case Type::Int:
      switch( rhs->type.kind ) {
        // int / int
        case Type::Int:
          if( rhs->as<Int>()->value == 0 )
            Error(expr->token)
              .setMessage("divided by zero")
              .emit()
              .exit();

          lhs->as<Int>()->value /= rhs->as<Int>()->value;
          break;

        // int / float
        case Type::Float:
        in_op_div_int_float:
          if( rhs->as<Float>()->value == 0 )
            Error(expr->token)
              .setMessage("divided by zero")
              .emit()
              .exit();

          lhs->as<Int>()->value /= (Int::ValueType)rhs->as<Float>()->value;
          break;

        // int / usize
        case Type::USize:
        in_op_div_int_usize:
          if( rhs->as<USize>()->value == 0 )
            Error(expr->token)
              .setMessage("divided by zero")
              .emit()
              .exit();

          lhs->as<Int>()->value /= (Int::ValueType)rhs->as<USize>()->value;
          break;

        default:
          goto invalidOperator;
      }
      break;

    case Type::Float:
      switch( rhs->type.kind ) {
        // float / int
        case Type::Int:
          std::swap(lhs, rhs);
          goto in_op_div_int_float;
        
        // float / float
        case Type::Float:
          if( rhs->as<Float>()->value == 0 )
            Error(expr->token)
              .setMessage("divided by zero")
              .emit()
              .exit();

          lhs->as<Float>()->value /= rhs->as<Float>()->value;
          break;

        // float / usize
        case Type::USize:
        in_op_div_float_usize:
          if( rhs->as<USize>()->value == 0 )
            Error(expr->token)
              .setMessage("divided by zero")
              .emit()
              .exit();

          lhs->as<Float>()->value /= (Float::ValueType)rhs->as<USize>()->value;
          break;

        default:
          goto invalidOperator;
      }
      break;

    case Type::USize:
      switch( rhs->type.kind ) {
        // usize / int
        case Type::Int:
          std::swap(lhs, rhs);
          goto in_op_div_int_usize;

        // usize / float
        case Type::Float:
          std::swap(lhs, rhs);
          goto in_op_div_float_usize;
        
        // usize / usize
        case Type::USize:
          if( rhs->as<USize>()->value == 0 )
            Error(expr->token)
              .setMessage("divided by zero")
              .emit()
              .exit();

          lhs->as<USize>()->value /= rhs->as<USize>()->value;
          break;

        default:
          goto invalidOperator;
      }
      break;

    default:
      goto invalidOperator;
  }
  goto end_label;

op_mod:
  switch( lhs->type.kind ) {
    case Type::Int:
      switch( rhs->type.kind ) {
        // int % int
        case Type::Int:
          lhs->as<Int>()->value %= rhs->as<Int>()->value;
          break;
        
        // int % float
        case Type::Float:
        in_op_mod_int_float:
          lhs->as<Int>()->value %= (Int::ValueType)rhs->as<Float>()->value;
          break;

        // int % usize
        case Type::USize:
        in_op_mod_int_usize:
          lhs->as<Int>()->value %= (Int::ValueType)rhs->as<USize>()->value;
          break;
      }
      break;

    case Type::Float:
      switch( rhs->type.kind ) {
        // float % int
        case Type::Int:
          std::swap(lhs, rhs);
          goto in_op_mod_int_float;

        // float % float
        // => int
        case Type::Float:
          lhs = new Int(
            (Int::ValueType)lhs->as<Float>()->value % (Int::ValueType)rhs->as<Float>()->value);
          break;

        // float % usize
        case Type::USize:
          std::swap(lhs, rhs);
          goto in_op_mod_usize_float;

        default:
          goto invalidOperator;
      }

    case Type::USize:
      switch( rhs->type.kind ) {
        // usize % int
        case Type::Int:
          std::swap(lhs, rhs);
          goto in_op_mod_int_usize;

        // usize % float
        case Type::Float:
        in_op_mod_usize_float:
          lhs->as<USize>()->value %= (USize::ValueType)rhs->as<Float>()->value;
          break;
        
        // usize % usize
        case Type::USize:
          lhs->as<USize>()->value %= rhs->as<USize>()->value;
          break;
      }

    default:
      goto invalidOperator;
  }
  goto end_label;

op_lshift:
  switch( lhs->type.kind ) {

    default:
      goto invalidOperator;
  }
  goto end_label;

op_rshift:
  switch( lhs->type.kind ) {

    default:
      goto invalidOperator;
  }
  goto end_label;

end_label:
  return lhs;
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
        .setMessage("expected 'usize' object")
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
    .setMessage("object of type '" + obj->type.to_string() + "' is not subscriptable")
    .emit()
    .exit();
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


} // namespace metro