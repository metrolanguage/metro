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

/*
 * obj_add
 */
Object* obj_add(AST::Expr* expr, Object* lhs, Object* rhs) {
  (void)expr;

  switch( lhs->type.kind ) {
    //
    // left is Int
    //
    case TypeInfo::Int: {
      switch( rhs->type.kind ) {
        // int + int
        case TypeInfo::Int:
          return new Int(lhs->as<Int>()->value + rhs->as<Int>()->value);

        // int + float
        case TypeInfo::Float:
          return new Int(lhs->as<Int>()->value + (Int::ValueType)rhs->as<Float>()->value);

        // int + usize
        case TypeInfo::USize:
          return new Int(lhs->as<Int>()->value + (Int::ValueType)rhs->as<USize>()->value);
      }

      break;
    }

    //
    // left is float
    //
    case TypeInfo::Float: {
      switch( rhs->type.kind ) {
        // float + int
        case TypeInfo::Int:
          return obj_add(expr, rhs, lhs);
        
        // float + float
        case TypeInfo::Float:
          return new Float(lhs->as<Float>()->value + rhs->as<Float>()->value);
        
        // float + usize
        case TypeInfo::USize:
          return new Float(lhs->as<Float>()->value + (Float::ValueType)rhs->as<Float>()->value);
      }

      break;
    }

    case TypeInfo::String:
      switch( rhs->type.kind ) {
        // string + char
        case TypeInfo::Char:
          return lhs->as<String>()->clone()->append(rhs->as<Char>());

        // string + string
        case TypeInfo::String:
          return lhs->as<String>()->clone()->append(rhs->as<String>());
      }
      break;

    case TypeInfo::Char:
      switch( rhs->type.kind ) {
        case TypeInfo::String: {
          rhs->as<String>()->value.insert(rhs->as<String>()->value.begin(), lhs->as<Char>()->clone());
          return rhs;
        }
      }

      break;
  }

  return nullptr;
}

/*
 * obj_sub
 */
Object* obj_sub(AST::Expr* expr, Object* lhs, Object* rhs) {
  (void)expr;
  
  switch( lhs->type.kind ) {
    // left is Int
    case TypeInfo::Int: {
      switch( rhs->type.kind ) {
        // int - int
        case TypeInfo::Int:
          return new Int(lhs->as<Int>()->value - rhs->as<Int>()->value);

        // int - float
        case TypeInfo::Float:
          return new Int(lhs->as<Int>()->value - (Int::ValueType)rhs->as<Float>()->value);

        // int - usize
        case TypeInfo::USize:
          return new Int(lhs->as<Int>()->value - (Int::ValueType)rhs->as<USize>()->value);
      }

      break;
    }

    // left is float
    case TypeInfo::Float: {
      switch( rhs->type.kind ) {
        // float - int
        case TypeInfo::Int:
          return new Float(lhs->as<Float>()->value - rhs->as<Int>()->value);

        // float - float
        case TypeInfo::Float:
          return new Float(lhs->as<Float>()->value - rhs->as<Float>()->value);

        // float - usize
        case TypeInfo::USize:
          return new Float(lhs->as<Float>()->value - (Float::ValueType)rhs->as<Float>()->value);
      }

      break;
    }
  }

  return nullptr;
}

/*
 * obj_mul
 */
Object* obj_mul(AST::Expr* expr, Object* lhs, Object* rhs) {
  (void)expr;

  switch( lhs->type.kind ) {
    case TypeInfo::Int:
      switch( rhs->type.kind ) {
        // int * int
        case TypeInfo::Int:
          return new Int(lhs->as<Int>()->value * rhs->as<Int>()->value);

        // int * float
        case TypeInfo::Float:
          return new Int(lhs->as<Int>()->value * (Int::ValueType)rhs->as<Float>()->value);

        // int * usize
        case TypeInfo::USize:
          return new Int(lhs->as<Int>()->value *= (Int::ValueType)rhs->as<USize>()->value);

        // int * string  // --> jump to (usize * string)
        // int * vector // --> jump to (usize * vector)
        case TypeInfo::String:
        case TypeInfo::Vector:
          return obj_mul(expr, new USize(lhs->as<Int>()->value), rhs);
      }
      break;

    case TypeInfo::Float:
      switch( rhs->type.kind ) {
        // float * int
        case TypeInfo::Int:
          return obj_mul(expr, rhs, lhs);

        // float * float
        case TypeInfo::Float:
          return new Float(lhs->as<Float>()->value * rhs->as<Float>()->value);
        
        // float * usize
        case TypeInfo::USize:
          return new Float(lhs->as<Float>()->value * (Float::ValueType)rhs->as<USize>()->value);
      }
      break;
    
    case TypeInfo::USize:
      switch( rhs->type.kind ) {
        // usize * int
        // usize * float
        case TypeInfo::Int:
        case TypeInfo::Float:
          return obj_mul(expr, rhs, lhs);

        // usize * usize
        case TypeInfo::USize:
          return new USize(lhs->as<USize>()->value *= rhs->as<USize>()->value);

        // usize * string
        // => string
        case TypeInfo::String: {
          auto obj = new String();

          for( USize::ValueType i = 0; i < lhs->as<USize>()->value; i++ )
            for( auto&& c : rhs->as<String>()->value )
              obj->value.emplace_back(c->clone());

          return obj;
        }

        // usize * vector
        case TypeInfo::Vector: {
          auto obj = new Vector();

          for( USize::ValueType i = 0; i < lhs->as<USize>()->value; i++ ) {
            for( auto&& e : rhs->as<Vector>()->elements )
              obj->elements.emplace_back(e->clone());
          }

          return obj;
        }
      }
      break;

    case TypeInfo::String:
      switch( rhs->type.kind ) {
        // string * int
        // string * usize
        case TypeInfo::Int:
        case TypeInfo::USize:
          return obj_mul(expr, rhs, lhs);
      }
      break;

    case TypeInfo::Vector:
      switch( rhs->type.kind ) {
        // vector * int
        case TypeInfo::Int:
          return obj_mul(expr, new USize(rhs->as<Int>()->value), lhs);

        // vector * usize
        case TypeInfo::USize:
          return obj_mul(expr, rhs, lhs);
      }
      break;
  }

  return nullptr;
}

/*
 * obj_div
 */
Object* obj_div(AST::Expr* expr, Object* lhs, Object* rhs) {
  (void)expr;

  switch( lhs->type.kind ) {
    case TypeInfo::Int:
      switch( rhs->type.kind ) {
        // int / int
        case TypeInfo::Int:
          if( rhs->as<Int>()->value == 0 )
            Error(expr->token)
              .setMessage("divided by zero")
              .emit()
              .exit();

          return new Int(lhs->as<Int>()->value / rhs->as<Int>()->value);

        // int / float
        case TypeInfo::Float:
          if( rhs->as<Float>()->value == 0 )
            Error(expr->token)
              .setMessage("divided by zero")
              .emit()
              .exit();

          return new Int(lhs->as<Int>()->value / (Int::ValueType)rhs->as<Float>()->value);

        // int / usize
        case TypeInfo::USize:
          if( rhs->as<USize>()->value == 0 )
            Error(expr->token)
              .setMessage("divided by zero")
              .emit()
              .exit();

          return new Int(lhs->as<Int>()->value / (Int::ValueType)rhs->as<USize>()->value);
      }
      break;

    case TypeInfo::Float:
      switch( rhs->type.kind ) {
        // float / int
        case TypeInfo::Int:
          return obj_mul(expr, rhs, lhs);

        // float / float
        case TypeInfo::Float:
          if( rhs->as<Float>()->value == 0 )
            Error(expr->token)
              .setMessage("divided by zero")
              .emit()
              .exit();

          return new Float(lhs->as<Float>()->value / rhs->as<Float>()->value);

        // float / usize
        case TypeInfo::USize: {
          if( rhs->as<USize>()->value == 0 )
            Error(expr->token)
              .setMessage("divided by zero")
              .emit()
              .exit();

          return new Float(lhs->as<Float>()->value / (Float::ValueType)rhs->as<USize>()->value);
        }
      }
      break;

    case TypeInfo::USize:
      switch( rhs->type.kind ) {
        // usize / int
        case TypeInfo::Int:
        case TypeInfo::Float:
          return obj_div(expr, rhs, lhs);

        // usize / usize
        case TypeInfo::USize: {
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

/*
 * obj_mod
 */
Object* obj_mod(AST::Expr* expr, Object* lhs, Object* rhs) {
  (void)expr;

  switch( lhs->type.kind ) {
    case TypeInfo::Int:
      switch( rhs->type.kind ) {
        // int % int
        case TypeInfo::Int:
          return new Int(lhs->as<Int>()->value % rhs->as<Int>()->value);
        
        // int % float
        case TypeInfo::Float:
          return new Int(lhs->as<Int>()->value % (Int::ValueType)rhs->as<Float>()->value);

        // int % usize
        case TypeInfo::USize:
          return new Int(lhs->as<Int>()->value % (Int::ValueType)rhs->as<USize>()->value);
      }
      break;

    case TypeInfo::Float:
      switch( rhs->type.kind ) {
        // float % int
        case TypeInfo::Int:
        case TypeInfo::USize:
          return obj_mod(expr, rhs, lhs);

        // float % float
        // => int
        case TypeInfo::Float:
          return new Int(
            (Int::ValueType)lhs->as<Float>()->value % (Int::ValueType)rhs->as<Float>()->value);
      }
      break;

    case TypeInfo::USize:
      switch( rhs->type.kind ) {
        // usize % int
        case TypeInfo::Int:
          return obj_mod(expr, rhs, lhs);

        // usize % float
        case TypeInfo::Float:
          return new USize(lhs->as<USize>()->value % (USize::ValueType)rhs->as<Float>()->value);
        
        // usize % usize
        case TypeInfo::USize:
          return new USize(lhs->as<USize>()->value % rhs->as<USize>()->value);
      }
      break;
  }

  return nullptr;
}

/*
 * obj_lshift
 */
Object* obj_lshift(AST::Expr* expr, Object* lhs, Object* rhs) {
  (void)expr;

  switch( lhs->type.kind ) {
    case TypeInfo::Int:
      switch( rhs->type.kind ) {
        // int << int
        case TypeInfo::Int:
          return new Int(lhs->as<Int>()->value << rhs->as<Int>()->value);
        
        // int << usize
        case TypeInfo::USize:
          return new Int(lhs->as<Int>()->value << rhs->as<USize>()->value);
      }
      break;

    case TypeInfo::USize:
      switch( rhs->type.kind ) {
        // usize << int
        case TypeInfo::Int:
          return new USize(lhs->as<USize>()->value << rhs->as<Int>()->value);
        
        // usize << usize
        case TypeInfo::USize:
          return new USize(lhs->as<USize>()->value << rhs->as<USize>()->value);
      }
      break;
  }

  return nullptr;
}

/*
 * obj_rshift
 */
Object* obj_rshift(AST::Expr* expr, Object* lhs, Object* rhs) {
  (void)expr;
  
  switch( lhs->type.kind ) {
    case TypeInfo::Int:
      switch( rhs->type.kind ) {
        // int << int
        case TypeInfo::Int:
          return new Int(lhs->as<Int>()->value >> rhs->as<Int>()->value);
        
        // int << usize
        case TypeInfo::USize:
          return new Int(lhs->as<Int>()->value >> rhs->as<USize>()->value);
      }
      break;

    case TypeInfo::USize:
      switch( rhs->type.kind ) {
        // usize << int
        case TypeInfo::Int:
          return new USize(lhs->as<USize>()->value >> rhs->as<Int>()->value);
        
        // usize << usize
        case TypeInfo::USize:
          return new USize(lhs->as<USize>()->value >> rhs->as<USize>()->value);
      }
      break;
  }

  return nullptr;
}

/*
 * obj_bigger
 */
Object* obj_bigger(AST::Expr* expr, Object* lhs, Object* rhs) {
  (void)expr;

  switch( lhs->type.kind ) {
    case TypeInfo::Int:
      switch( rhs->type.kind ) {
        // int > int
        case TypeInfo::Int:
          return new Bool(lhs->as<Int>()->value > rhs->as<Int>()->value);

        // int > float
        case TypeInfo::Float:
          return new Bool(lhs->as<Int>()->value > (Int::ValueType)rhs->as<Float>()->value);

        // int > usize
        case TypeInfo::USize:
          return new Bool(lhs->as<Int>()->value > (Int::ValueType)rhs->as<USize>()->value);
      }
      break;

    case TypeInfo::Float:
      switch( rhs->type.kind ) {
        // float > int
        case TypeInfo::Int:
          return new Bool(lhs->as<Float>()->value > rhs->as<Int>()->value);

        // float > float
        case TypeInfo::Float:
          return new Bool(lhs->as<Float>()->value > (Float::ValueType)rhs->as<Float>()->value);

        // float > usize
        case TypeInfo::USize:
          return new Bool(lhs->as<Float>()->value > (Float::ValueType)rhs->as<USize>()->value);
      }
      break;

    case TypeInfo::USize:
      switch( rhs->type.kind ) {
        // usize > int
        case TypeInfo::Int:
          return new Bool(lhs->as<USize>()->value > (unsigned)rhs->as<Int>()->value);

        // usize > float
        case TypeInfo::Float:
          return new Bool(lhs->as<USize>()->value > (USize::ValueType)rhs->as<Float>()->value);

        // usize > usize
        case TypeInfo::USize:
          return new Bool(lhs->as<USize>()->value > (USize::ValueType)rhs->as<USize>()->value);
      }
      break;
  }

  return nullptr;
}

/*
 * obj_bigger_or_equal
 */
Object* obj_bigger_or_equal(AST::Expr* expr, Object* lhs, Object* rhs) {
  (void)expr;

  switch( lhs->type.kind ) {
    case TypeInfo::Int:
      switch( rhs->type.kind ) {
        // int >= int
        case TypeInfo::Int:
          return new Bool(lhs->as<Int>()->value >= rhs->as<Int>()->value);

        // int >= float
        case TypeInfo::Float:
          return new Bool(lhs->as<Int>()->value >= (Int::ValueType)rhs->as<Float>()->value);

        // int >= usize
        case TypeInfo::USize:
          return new Bool(lhs->as<Int>()->value >= (Int::ValueType)rhs->as<USize>()->value);
      }
      break;

    case TypeInfo::Float:
      switch( rhs->type.kind ) {
        // float >= int
        case TypeInfo::Int:
          return new Bool(lhs->as<Float>()->value >= rhs->as<Int>()->value);

        // float >= float
        case TypeInfo::Float:
          return new Bool(lhs->as<Float>()->value >= (Float::ValueType)rhs->as<Float>()->value);

        // float >= usize
        case TypeInfo::USize:
          return new Bool(lhs->as<Float>()->value >= (Float::ValueType)rhs->as<USize>()->value);
      }
      break;

    case TypeInfo::USize:
      switch( rhs->type.kind ) {
        // usize >= int
        case TypeInfo::Int:
          return new Bool(lhs->as<USize>()->value >= (unsigned)rhs->as<Int>()->value);

        // usize >= float
        case TypeInfo::Float:
          return new Bool(lhs->as<USize>()->value >= (USize::ValueType)rhs->as<Float>()->value);

        // usize >= usize
        case TypeInfo::USize:
          return new Bool(lhs->as<USize>()->value >= (USize::ValueType)rhs->as<USize>()->value);
      }
      break;
  }

  return nullptr;
}

/*
 * obj_bit_and
 */
Object* obj_bit_and(AST::Expr* expr, Object* lhs, Object* rhs) {
  (void)expr;

  switch( lhs->type.kind ) {
    case TypeInfo::Int:
      switch( rhs->type.kind ) {
        // int & int
        case TypeInfo::Int:
          return new Int(lhs->as<Int>()->value & rhs->as<Int>()->value);
        
        // int & usize
        case TypeInfo::USize:
          return new Int(lhs->as<Int>()->value & rhs->as<USize>()->value);
      }
      break;

    case TypeInfo::USize:
      switch( rhs->type.kind ) {
        // usize & int
        case TypeInfo::Int:
          return new USize(lhs->as<USize>()->value & rhs->as<Int>()->value);
        
        // usize & usize
        case TypeInfo::USize:
          return new USize(lhs->as<USize>()->value & rhs->as<USize>()->value);
      }
      break;
  }

  return nullptr;
}

/*
 * obj_bit_xor
 */
Object* obj_bit_xor(AST::Expr* expr, Object* lhs, Object* rhs) {
  (void)expr;

  switch( lhs->type.kind ) {
    case TypeInfo::Int:
      switch( rhs->type.kind ) {
        // int & int
        case TypeInfo::Int:
          return new Int(lhs->as<Int>()->value ^ rhs->as<Int>()->value);
        
        // int ^ usize
        case TypeInfo::USize:
          return new Int(lhs->as<Int>()->value ^ rhs->as<USize>()->value);
      }
      break;

    case TypeInfo::USize:
      switch( rhs->type.kind ) {
        // usize ^ int
        case TypeInfo::Int:
          return new USize(lhs->as<USize>()->value ^ rhs->as<Int>()->value);
        
        // usize ^ usize
        case TypeInfo::USize:
          return new USize(lhs->as<USize>()->value ^ rhs->as<USize>()->value);
      }
      break;
  }

  return nullptr;
}

/*
 * obj_bit_or
 */
Object* obj_bit_or(AST::Expr* expr, Object* lhs, Object* rhs) {
  (void)expr;
  
  switch( lhs->type.kind ) {
    case TypeInfo::Int:
      switch( rhs->type.kind ) {
        // int | int
        case TypeInfo::Int:
          return new Int(lhs->as<Int>()->value | rhs->as<Int>()->value);
        
        // int | usize
        case TypeInfo::USize:
          return new Int(lhs->as<Int>()->value | rhs->as<USize>()->value);
      }
      break;

    case TypeInfo::USize:
      switch( rhs->type.kind ) {
        // usize | int
        case TypeInfo::Int:
          return new USize(lhs->as<USize>()->value | rhs->as<Int>()->value);
        
        // usize | usize
        case TypeInfo::USize:
          return new USize(lhs->as<USize>()->value | rhs->as<USize>()->value);
      }
      break;
  }

  return nullptr;
}

} // anonymous namespace

Object* Evaluator::evalOperator(AST::Expr* expr) {
  using opFuncPointer = Object*(*)(AST::Expr*, Object*, Object*);

  static opFuncPointer operator_labels[] = {
    nullptr, // value
    nullptr, // variable
    nullptr, // callfunc
    nullptr, // new

    nullptr, // array
    nullptr, // tuple
    nullptr, // pair
    
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
    nullptr, // equal

    obj_bit_and,
    obj_bit_xor,
    obj_bit_or,
  };

  auto lhs = this->eval(expr->left);
  auto rhs = this->eval(expr->right);

  GC::bind(lhs);
  GC::bind(rhs);

  auto result =
    operator_labels[static_cast<int>(expr->kind)](expr, lhs, rhs);

  GC::unbind(lhs);
  GC::unbind(rhs);

  if( !result ) {
    Error(expr->token)
      .setMessage(
        "invalid operator: '" + lhs->type.to_string() + "' "
          + std::string(expr->token->str) + " '" + rhs->type.to_string() + "'")
      .emit()
      .exit();
  }

  return result;
}

} // namespace metro