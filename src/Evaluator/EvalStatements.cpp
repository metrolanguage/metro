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

    if( !cond->type.equals(TypeInfo::Bool) ) {
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

    (void)sw;

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

      if( !cond->type.equals(TypeInfo::Bool) )
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

    if( !content->type.is_iterable() ) {
      Error(x->content)
        .setMessage("object of type '" + content->type.to_string() + "' is not iterable")
        .emit()
        .exit();
    }

    switch( content->type.kind ) {
      case TypeInfo::String: {
        auto _Str = content->as<String>();

        for( auto&& _Char : _Str->value ) {
          iter = _Char;
          this->eval(x->code);
        }

        break;
      }

      case TypeInfo::Vector: {
        auto _Vec = content->as<Vector>();

        for( auto&& _Elem : _Vec->elements ) {
          iter = _Elem;
          this->eval(x->code);
        }

        break;
      }

      case TypeInfo::Dict: {
        todo_impl;
      }

      case TypeInfo::Range: {
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

} // namespace metro