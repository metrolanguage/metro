#pragma once

#include <map>
#include "ast.h"
#include "object.h"

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

  objects::Object* eval(AST::Base* ast);
  objects::Object*& evalAsLeft(AST::Base* ast);


private:

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

  CallStack& push_stack(AST::Function const* func);
  void pop_stack();

  Storage  globalStorage;
  std::vector<CallStack> callStacks;

};

} // namespace metro