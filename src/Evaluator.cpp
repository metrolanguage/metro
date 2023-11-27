#include <iostream>
#include <sstream>
#include "alert.h"
#include "BuiltinFunc.h"
#include "Evaluator.h"
#include "GC.h"
#include "Utils.h"
#include "Error.h"

#define LABEL_TABLE   \
  static void* _labels[]

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
          .setMessage("undefined variable")
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

        if( !storage.contains(name) ) {
          GC::bind(storage[name] = value);
          return value;
        }
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

void Evaluator::evalStatements(AST::Base* ast) {
  LABEL_TABLE {
    &&_eval_scope,
    &&_eval_if,
    &&_eval_switch,
    &&_eval_return,
    &&_eval_break,
    &&_eval_continue,
    &&_eval_loop,
    &&_eval_while,
    &&_eval_do_while,
    &&_eval_for,
  };

  goto *_labels[static_cast<int>(ast->kind) - static_cast<int>(ASTKind::Scope)];

  _eval_scope: {
    auto scope = ast->as<AST::Scope>();

    ScopeEvaluationFlags
      flags, *save = this->_scope;

    this->_scope = &flags;

    for( auto&& x : scope->list ) {
      this->eval(x);

      if( this->inFunction() ) {
          if( this->getCurrentCallStack().isReturned ) {
          break;
        }
      }

      if( _loopScope ) {
        if( _loopScope->isBreaked || _loopScope->isContinued )
          break;
      }
    }

    this->_scope = save;

    goto _end;
  }

  _eval_if: {
    auto x = ast->as<AST::If>();

    auto cond = this->eval(x->cond);

    if( !cond->type.equals(Type::Bool) ) {
      Error(x->cond)
        .setMessage("expected boolean expression")
        .emit()
        .exit();
    }

    if( cond->as<Bool>()->value )
      this->eval(x->case_true);
    else if( x->case_false )
      this->eval(x->case_false);

    goto _end;
  }

  _eval_switch: {
    auto sw = ast->as<AST::Switch>();


    goto _end;
  }
  
  _eval_return: {
    if( !this->inFunction() ) {
      Error(ast)
        .setMessage("cannot use 'return' out side of function")
        .emit()
        .exit();
    }

    auto& stack = this->getCurrentCallStack();

    stack.isReturned = true;

    if( auto expr = ast->as<AST::Expr>()->left; expr )
      stack.result = this->eval(expr);

    goto _end;
  }
  
  _eval_break:
    todo_impl;
  
  _eval_continue:
    todo_impl;
  
  _eval_loop:
    todo_impl;
  
  _eval_while: {
    auto x = ast->as<AST::While>();

    while( true ) {
      auto cond = this->eval(x->cond);

      if( !cond->type.equals(Type::Bool) )
        Error(x->cond)
          .setMessage("expected boolean expression")
          .emit()
          .exit();
      
      if( !cond->as<Bool>()->value )
        break;

      this->eval(x->code);
    }

    goto _end;
  }
  
  _eval_do_while:
    todo_impl;

  _eval_for: {
    auto x = ast->as<AST::For>();

    // if already defined variable with same name of iterator, save object of that.
    Object* save_iter_value = nullptr;
    Object** saved_var_ptr = nullptr;

    auto& iter = this->evalAsLeft(x->iter);
    auto content = this->eval(x->content);

    if( iter ) {
      save_iter_value = iter;
      saved_var_ptr = &iter;
    }

    if( !content->type.isIterable() ) {
      Error(x->content)
        .setMessage("object of type '" + content->type.toString() + "' is not iterable")
        .emit()
        .exit();
    }

    switch( content->type.kind ) {
      case Type::String: {
        auto _Str = content->as<String>();

        for( auto&& _Char : _Str->value ) {
          iter = _Char;
          this->eval(x->code);
        }

        break;
      }

      case Type::Vector: {
        auto _Vec = content->as<Vector>();

        for( auto&& _Elem : _Vec->elements ) {
          iter = _Elem;
          this->eval(x->code);
        }

        break;
      }

      case Type::Dict: {
        todo_impl;
      }

      case Type::Range: {
        auto range = content->as<Range>();

        for( iter = new Int(range->begin); iter->as<Int>()->value < range->end; iter->as<Int>()->value++ ) {
          this->eval(x->code);
        }

        break;
      }
    }

    // restore if saved
    if( saved_var_ptr ) {
      alert;
      *saved_var_ptr = save_iter_value;
    }
  }

  _end:;
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
// === evalOperator ===
//
Object* Evaluator::evalOperator(AST::Expr* expr) {
  static void* operator_labels[] = {
    nullptr, // value
    nullptr, // variable
    nullptr, // callfunc
    nullptr, // array
    nullptr, // tuple
    nullptr, // memberaccess
    nullptr, // indexref
    nullptr, // not

    &&op_add,
    &&op_sub,
    &&op_mul,
    &&op_div,
    &&op_mod,
    &&op_lshift,
    &&op_rshift,

    nullptr, // range

    &&op_bigger,
    &&op_bigger_or_equal,
    &&op_equal,

    &&op_bit_and,
    &&op_bit_xor,
    &&op_bit_or,
  };

  auto lhs = this->eval(expr->left);
  auto rhs = this->eval(expr->right);
  auto ret = lhs;

  GC::bind(lhs);
  GC::bind(rhs);

  if( static_cast<size_t>(expr->kind) >= std::size(operator_labels) )
    todo_impl;

  goto *operator_labels[static_cast<int>(expr->kind)];

  invalidOperator:
    Error(expr->token)
      .setMessage(
        "invalid operator: '" + lhs->type.toString() + "' "
          + std::string(expr->token->str) + " '" + rhs->type.toString() + "'")
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
          lhs->as<Float>()->value -= rhs->as<Int>()->value;
          break;
        
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
    case Type::Int:
      switch( rhs->type.kind ) {
        // int << int
        case Type::Int:
          lhs->as<Int>()->value <<= rhs->as<Int>()->value;
          break;
        
        // int << usize
        case Type::USize:
          lhs->as<Int>()->value <<= rhs->as<USize>()->value;
          break;

        default:
          goto invalidOperator;
      }
      break;

    case Type::USize:
      switch( rhs->type.kind ) {
        // usize << int
        case Type::Int:
          lhs->as<USize>()->value <<= rhs->as<Int>()->value;
          break;
        
        // usize << usize
        case Type::USize:
          lhs->as<USize>()->value <<= rhs->as<USize>()->value;
          break;

        default:
          goto invalidOperator;
      }
      break;

    default:
      goto invalidOperator;
  }
  goto end_label;

op_rshift:
  switch( lhs->type.kind ) {
    case Type::Int:
      switch( rhs->type.kind ) {
        // int >> int
        case Type::Int:
          lhs->as<Int>()->value >>= rhs->as<Int>()->value;
          break;
        
        // int >> usize
        case Type::USize:
          lhs->as<Int>()->value >>= rhs->as<USize>()->value;
          break;

        default:
          goto invalidOperator;
      }
      break;

    case Type::USize:
      switch( rhs->type.kind ) {
        // usize >> int
        case Type::Int:
          lhs->as<USize>()->value >>= rhs->as<Int>()->value;
          break;
        
        // usize >> usize
        case Type::USize:
          lhs->as<USize>()->value >>= rhs->as<USize>()->value;
          break;

        default:
          goto invalidOperator;
      }
      break;

    default:
      goto invalidOperator;
  }
  goto end_label;

op_bigger:
  switch( lhs->type.kind ) {
    case Type::Int:
      switch( rhs->type.kind ) {
        // int > int
        case Type::Int:
          ret = new Bool(lhs->as<Int>()->value > rhs->as<Int>()->value);
          break;

        // int > float
        case Type::Float:
          ret = new Bool(lhs->as<Int>()->value > (Int::ValueType)rhs->as<Float>()->value);
          break;

        // int > usize
        case Type::USize:
          ret = new Bool(lhs->as<Int>()->value > (Int::ValueType)rhs->as<USize>()->value);
          break;
          
        default:
          goto invalidOperator;
      }
      break;

    case Type::Float:
      switch( rhs->type.kind ) {
        // float > int
        case Type::Int:
          ret = new Bool(lhs->as<Float>()->value > rhs->as<Int>()->value);
          break;

        // float > float
        case Type::Float:
          ret = new Bool(lhs->as<Float>()->value > (Float::ValueType)rhs->as<Float>()->value);
          break;

        // float > usize
        case Type::USize:
          ret = new Bool(lhs->as<Float>()->value > (Float::ValueType)rhs->as<USize>()->value);
          break;
          
        default:
          goto invalidOperator;
      }
      break;

    case Type::USize:
      switch( rhs->type.kind ) {
        // usize > int
        case Type::Int:
          ret = new Bool(lhs->as<USize>()->value > (unsigned)rhs->as<Int>()->value);
          break;

        // usize > float
        case Type::Float:
          ret = new Bool(lhs->as<USize>()->value > (USize::ValueType)rhs->as<Float>()->value);
          break;

        // usize > usize
        case Type::USize:
          ret = new Bool(lhs->as<USize>()->value > (USize::ValueType)rhs->as<USize>()->value);
          break;

        default:
         goto invalidOperator;
      }
      break;

    default:
      goto invalidOperator;
  }
  goto end_label;

op_bigger_or_equal:
  switch( lhs->type.kind ) {
    case Type::Int:
      switch( rhs->type.kind ) {
        // int >= int
        case Type::Int:
          ret = new Bool(lhs->as<Int>()->value >= rhs->as<Int>()->value);
          break;

        // int >= float
        case Type::Float:
          ret = new Bool(lhs->as<Int>()->value >= (Int::ValueType)rhs->as<Float>()->value);
          break;

        // int >= usize
        case Type::USize:
          ret = new Bool(lhs->as<Int>()->value >= (Int::ValueType)rhs->as<USize>()->value);
          break;
          
        default:
          goto invalidOperator;
      }
      break;

    case Type::Float:
      switch( rhs->type.kind ) {
        // float >= int
        case Type::Int:
          ret = new Bool(lhs->as<Float>()->value >= rhs->as<Int>()->value);
          break;

        // float >= float
        case Type::Float:
          ret = new Bool(lhs->as<Float>()->value >= (Float::ValueType)rhs->as<Float>()->value);
          break;

        // float >= usize
        case Type::USize:
          ret = new Bool(lhs->as<Float>()->value >= (Float::ValueType)rhs->as<USize>()->value);
          break;
          
        default:
          goto invalidOperator;
      }
      break;

    case Type::USize:
      switch( rhs->type.kind ) {
        // usize >= int
        case Type::Int:
          ret = new Bool(lhs->as<USize>()->value >= (unsigned)rhs->as<Int>()->value);
          break;

        // usize >= float
        case Type::Float:
          ret = new Bool(lhs->as<USize>()->value >= (USize::ValueType)rhs->as<Float>()->value);
          break;

        // usize >= usize
        case Type::USize:
          ret = new Bool(lhs->as<USize>()->value >= (USize::ValueType)rhs->as<USize>()->value);
          break;

        default:
         goto invalidOperator;
      }
      break;

    default:
      goto invalidOperator;
  }
  goto end_label;

op_equal:
  switch( lhs->type.kind ) {
    case Type::Int:
      switch( rhs->type.kind ) {
        // int == int
        case Type::Int:
          ret = new Bool(lhs->as<Int>()->value == rhs->as<Int>()->value);
          break;

        // int == float
        case Type::Float:
          in_op_equal_int_float:
          ret = new Bool(lhs->as<Int>()->value == rhs->as<Float>()->value);
          break;

        // int == usize
        case Type::USize:
          in_op_equal_int_usize:
          ret = new Bool((unsigned)lhs->as<Int>()->value == rhs->as<USize>()->value);
          break;

        default:
          goto invalidOperator;
      }
      break;

    case Type::Float:
      switch( rhs->type.kind ) {
        // float == int
        case Type::Int:
          std::swap(lhs, rhs);
          goto in_op_equal_int_float;
        
        // float == float
        case Type::Float:
          ret = new Bool(lhs->as<Float>()->value == rhs->as<Float>()->value);
          break;

        // float == usize
        case Type::USize:
        in_op_equal_float_usize:
          ret = new Bool(lhs->as<Float>()->value == rhs->as<USize>()->value);
          break;

        default:
          goto invalidOperator;
      }
      break;
    
    case Type::USize:
      switch( rhs->type.kind ) {
        // usize == int
        case Type::Int:
          std::swap(lhs, rhs);
          goto in_op_equal_int_usize;

        // usize == float
        case Type::Float:
          std::swap(lhs, rhs);
          goto in_op_equal_float_usize;
        
        // usize == usize
        case Type::USize:
          ret = new Bool(lhs->as<USize>()->value == rhs->as<USize>()->value);
          break;

        default:
          goto invalidOperator;
      }
      break;

    default:
      goto invalidOperator;
  }
  goto end_label;

op_bit_and:
  switch( lhs->type.kind ) {
    case Type::Int:
      switch( rhs->type.kind ) {
        // int & int
        case Type::Int:
          lhs->as<Int>()->value &= rhs->as<Int>()->value;
          break;
        
        // int & usize
        case Type::USize:
          lhs->as<Int>()->value &= rhs->as<USize>()->value;
          break;

        default:
          goto invalidOperator;
      }
      break;

    case Type::USize:
      switch( rhs->type.kind ) {
        // usize & int
        case Type::Int:
          lhs->as<USize>()->value &= rhs->as<Int>()->value;
          break;
        
        // usize & usize
        case Type::USize:
          lhs->as<USize>()->value &= rhs->as<USize>()->value;
          break;

        default:
          goto invalidOperator;
      }
      break;

    default:
      goto invalidOperator;
  }
  goto end_label;

op_bit_xor:
  switch( lhs->type.kind ) {
    case Type::Int:
      switch( rhs->type.kind ) {
        // int ^ int
        case Type::Int:
          lhs->as<Int>()->value ^= rhs->as<Int>()->value;
          break;
        
        // int ^ usize
        case Type::USize:
          lhs->as<Int>()->value ^= rhs->as<USize>()->value;
          break;

        default:
          goto invalidOperator;
      }
      break;

    case Type::USize:
      switch( rhs->type.kind ) {
        // usize ^ int
        case Type::Int:
          lhs->as<USize>()->value ^= rhs->as<Int>()->value;
          break;
        
        // usize ^ usize
        case Type::USize:
          lhs->as<USize>()->value ^= rhs->as<USize>()->value;
          break;

        default:
          goto invalidOperator;
      }
      break;

    default:
      goto invalidOperator;
  }
  goto end_label;

op_bit_or:
  switch( lhs->type.kind ) {
    case Type::Int:
      switch( rhs->type.kind ) {
        // int | int
        case Type::Int:
          lhs->as<Int>()->value |= rhs->as<Int>()->value;
          break;
        
        // int | usize
        case Type::USize:
          lhs->as<Int>()->value |= rhs->as<USize>()->value;
          break;

        default:
          goto invalidOperator;
      }
      break;

    case Type::USize:
      switch( rhs->type.kind ) {
        // usize | int
        case Type::Int:
          lhs->as<USize>()->value |= rhs->as<Int>()->value;
          break;
        
        // usize | usize
        case Type::USize:
          lhs->as<USize>()->value |= rhs->as<USize>()->value;
          break;

        default:
          goto invalidOperator;
      }
      break;

    default:
      goto invalidOperator;
  }
  goto end_label;

end_label:
  GC::unbind(lhs);
  GC::unbind(rhs);

  return ret;
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