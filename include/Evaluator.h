#pragma once

#include <map>
#include "AST.h"
#include "Object.h"

namespace metro {

class Evaluator {
  using Object = objects::Object;
  using Storage = std::map<std::string_view, Object*>;

  struct CallStack {
    AST::Function const* func;
    Object*  result;
    Storage  storage;

    CallStack(AST::Function const* func)
      : func(func),
        result(nullptr)
    {
    }
  };

public:

  Evaluator() { }

  /*
   * The core function.
   */
  Object* eval(AST::Base* ast);


  /*
   * evaluate a node as lvalue.
   * if can't be evaluated as lvalue, show error.
   */
  Object*& evalAsLeft(AST::Base* ast);


  /*
   * evaluate operator in expression
   */
  Object* evalOperator(AST::Expr* expr);

private:

  Object*& evalIndexRef(AST::Expr* ast, Object* obj, Object* index);

  CallStack& push_stack(AST::Function const* func);
  void pop_stack();

  bool in_func() const {
    return !this->callStacks.empty();
  }

  CallStack& getCurrentCallStack() {
    return *callStacks.rbegin();
  }

  Storage& getCurrentStorage() {
    if( this->in_func() )
      return this->getCurrentCallStack().storage;

    return this->globalStorage;
  }

  Storage  globalStorage;
  std::vector<CallStack> callStacks;

};

} // namespace metro