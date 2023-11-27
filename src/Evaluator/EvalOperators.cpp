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

} // namespace metro