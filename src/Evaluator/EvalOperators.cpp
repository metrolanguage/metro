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

namespace {

Object* obj_add(AST::Expr* expr, Object* lhs, Object* rhs) {
  switch( lhs->type.kind ) {
    //
    // left is Int
    //
    case Type::Int: {
      switch( rhs->type.kind ) {
        // int + int
        case Type::Int:
          return new Int(lhs->as<Int>()->value + rhs->as<Int>()->value);

        // int + float
        case Type::Float:
          return new Int(lhs->as<Int>()->value + (Int::ValueType)rhs->as<Float>()->value);

        // int + usize
        case Type::USize:
          return new Int(lhs->as<Int>()->value + (Int::ValueType)rhs->as<USize>()->value);
      }

      break;
    }

    //
    // left is float
    //
    case Type::Float: {
      switch( rhs->type.kind ) {
        // float + int
        case Type::Int:
          return obj_add(expr, rhs, lhs);
        
        // float + float
        case Type::Float:
          return new Float(lhs->as<Float>()->value + rhs->as<Float>()->value);
        
        // float + usize
        case Type::USize:
          return new Float(lhs->as<Float>()->value + (Float::ValueType)rhs->as<Float>()->value);
      }

      break;
    }

    case Type::String:
      switch( rhs->type.kind ) {
        case Type::Char:
          return lhs->as<String>()->clone()->append(rhs->as<Char>());

        case Type::String:
          return lhs->as<String>()->clone()->append(rhs->as<String>());
      }
      break;
  }

  return nullptr;
}

Object* obj_sub(AST::Expr* expr, Object* lhs, Object* rhs) {
  switch( lhs->type.kind ) {
    // left is Int
    case Type::Int: {
      switch( rhs->type.kind ) {
        // int - int
        case Type::Int:
          return new Int(lhs->as<Int>()->value - rhs->as<Int>()->value);

        // int - float
        case Type::Float:
          return new Int(lhs->as<Int>()->value - (Int::ValueType)rhs->as<Float>()->value);

        // int - usize
        case Type::USize:
          return new Int(lhs->as<Int>()->value - (Int::ValueType)rhs->as<USize>()->value);
      }

      break;
    }

    // left is float
    case Type::Float: {
      switch( rhs->type.kind ) {
        // float - int
        case Type::Int:
          return new Float(lhs->as<Float>()->value - rhs->as<Int>()->value);
        
        // float - float
        case Type::Float:
          return new Float(lhs->as<Float>()->value - rhs->as<Float>()->value);
        
        // float - usize
        case Type::USize:
          return new Float(lhs->as<Float>()->value - (Float::ValueType)rhs->as<Float>()->value);
      }

      break;
    }
  }

  return nullptr;
}

Object* obj_mul(AST::Expr* expr, Object* lhs, Object* rhs) {
  switch( lhs->type.kind ) {
    case Type::Int:
      switch( rhs->type.kind ) {
        // int * int
        case Type::Int:
          return new Int(lhs->as<Int>()->value * rhs->as<Int>()->value);

        // int * float
        case Type::Float:
          return new Int(lhs->as<Int>()->value * (Int::ValueType)rhs->as<Float>()->value);

        // int * usize
        case Type::USize:
          return new Int(lhs->as<Int>()->value *= (Int::ValueType)rhs->as<USize>()->value);

        // int * string  // --> jump to (usize * string)
        // int * vector // --> jump to (usize * vector)
        case Type::String:
        case Type::Vector:
          return obj_mul(expr, new USize(lhs->as<Int>()->value), rhs);
      }
      break;

    case Type::Float:
      switch( rhs->type.kind ) {
        // float * int
        case Type::Int:
          return obj_mul(expr, rhs, lhs);

        // float * float
        case Type::Float:
          return new Float(lhs->as<Float>()->value * rhs->as<Float>()->value);
        
        // float * usize
        case Type::USize:
          return new Float(lhs->as<Float>()->value * (Float::ValueType)rhs->as<USize>()->value);
      }
      break;
    
    case Type::USize:
      switch( rhs->type.kind ) {
        // usize * int
        // usize * float
        case Type::Int:
        case Type::Float:
          return obj_mul(expr, rhs, lhs);

        // usize * usize
        case Type::USize:
          return new USize(lhs->as<USize>()->value *= rhs->as<USize>()->value);

        // usize * string
        // => string
        case Type::String: {
          auto obj = new String();

          for( USize::ValueType i = 0; i < lhs->as<USize>()->value; i++ )
            for( auto&& c : rhs->as<String>()->value )
              obj->value.emplace_back(c->clone());

          return obj;
        }

        // usize * vector
        case Type::Vector: {
          auto obj = new Vector();

          for( USize::ValueType i = 0; i < lhs->as<USize>()->value; i++ ) {
            for( auto&& e : rhs->as<Vector>()->elements )
              obj->elements.emplace_back(e->clone());
          }

          return obj;
        }
      }
      break;

    case Type::String:
      switch( rhs->type.kind ) {
        // string * int
        // string * usize
        case Type::Int:
        case Type::USize:
          return obj_mul(expr, rhs, lhs);
      }
      break;

    case Type::Vector:
      switch( rhs->type.kind ) {
        // vector * int
        case Type::Int:
          rhs = new USize(rhs->as<Int>()->value);

        // vector * usize
        case Type::USize:
          return obj_mul(expr, rhs, lhs);
      }
      break;
  }

  return nullptr;
}

Object* obj_div(AST::Expr* expr, Object* lhs, Object* rhs) {
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

          return new Int(lhs->as<Int>()->value / rhs->as<Int>()->value);

        // int / float
        case Type::Float:
          if( rhs->as<Float>()->value == 0 )
            Error(expr->token)
              .setMessage("divided by zero")
              .emit()
              .exit();

          return new Int(lhs->as<Int>()->value / (Int::ValueType)rhs->as<Float>()->value);

        // int / usize
        case Type::USize:
          if( rhs->as<USize>()->value == 0 )
            Error(expr->token)
              .setMessage("divided by zero")
              .emit()
              .exit();

          return new Int(lhs->as<Int>()->value / (Int::ValueType)rhs->as<USize>()->value);
      }
      break;

    case Type::Float:
      switch( rhs->type.kind ) {
        // float / int
        case Type::Int:
          return obj_mul(expr, rhs, lhs);

        // float / float
        case Type::Float:
          if( rhs->as<Float>()->value == 0 )
            Error(expr->token)
              .setMessage("divided by zero")
              .emit()
              .exit();

          return new Float(lhs->as<Float>()->value / rhs->as<Float>()->value);

        // float / usize
        case Type::USize: {
          if( rhs->as<USize>()->value == 0 )
            Error(expr->token)
              .setMessage("divided by zero")
              .emit()
              .exit();

          return new Float(lhs->as<Float>()->value / (Float::ValueType)rhs->as<USize>()->value);
        }
      }
      break;

    case Type::USize:
      switch( rhs->type.kind ) {
        // usize / int
        case Type::Int:
        case Type::Float:
          return obj_div(expr, rhs, lhs);

        // usize / usize
        case Type::USize: {
          if( rhs->as<USize>()->value == 0 )
            Error(expr->token)
              .setMessage("divided by zero")
              .emit()
              .exit();

          return new USize(lhs->as<USize>()->value / rhs->as<USize>()->value);
        }
      }
      break;
  }

  return nullptr;
}

Object* obj_mod(AST::Expr* expr, Object* lhs, Object* rhs) {

}

Object* obj_lshift(AST::Expr* expr, Object* lhs, Object* rhs) {

}

Object* obj_rshift(AST::Expr* expr, Object* lhs, Object* rhs) {

}

Object* obj_bigger(AST::Expr* expr, Object* lhs, Object* rhs) {

}

Object* obj_bigger_or_equal(AST::Expr* expr, Object* lhs, Object* rhs) {

}

Object* obj_equal(AST::Expr* expr, Object* lhs, Object* rhs) {

}

Object* obj_bit_and(AST::Expr* expr, Object* lhs, Object* rhs) {

}

Object* obj_bit_xor(AST::Expr* expr, Object* lhs, Object* rhs) {

}

Object* obj_bit_or(AST::Expr* expr, Object* lhs, Object* rhs) {

}

} // anonymous namespace

Object* Evaluator::evalOperator(AST::Expr* expr) {
  using opFuncPointer = Object*(*)(Object*, Object*);

  static opFuncPointer operator_labels[] = {
    nullptr, // value
    nullptr, // variable
    nullptr, // callfunc
    nullptr, // array
    nullptr, // tuple
    nullptr, // memberaccess
    nullptr, // indexref
    nullptr, // not

    obj_add,
    obj_sub,
    obj_mul,
    obj_div,
    obj_mod,
    obj_lshift,
    obj_rshift,

    nullptr, // range

    obj_bigger,
    obj_bigger_or_equal,
    obj_equal,

    obj_bit_and,
    obj_bit_xor,
    obj_bit_or,
  };

  auto lhs = this->eval(expr->left);
  auto rhs = this->eval(expr->right);

  GC::bind(lhs);
  GC::bind(rhs);

  auto result =
    operator_labels[static_cast<int>(expr->kind)](lhs, rhs);

  if( !result ) {
    Error(expr->token)
      .setMessage(
        "invalid operator: '" + lhs->type.toString() + "' "
          + std::string(expr->token->str) + " '" + rhs->type.toString() + "'")
      .emit()
      .exit();
  }

  return result;

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