#pragma once

#include "ast.h"
#include "object.h"

namespace metro {

class Evaluator {
  using Object = objects::Object;
  using Storage = std::vector<Object*>;

  struct CallStack {
    AST::Function* func;
    Storage  storage;

    CallStack(AST::Function* func)
      : func(func)
    {
    }
  };

public:

  Evaluator() { }

  objects::Object* eval(AST::Base* ast);


private:

  bool in_func() const {
    return !this->callStacks.empty();
  }

  CallStack& getCurrentCallStack() {
    return *callStacks.rbegin();
  }

  Storage  globalStorage;
  std::vector<CallStack> callStacks;

};

} // namespace metro