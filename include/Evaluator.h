#pragma once

#include <map>
#include <list>
#include "AST.h"
#include "Object.h"

namespace metro {

class Evaluator {
  using Object = objects::Object;
  using Storage = std::map<std::string_view, Object*>;

  struct CallStack {
    AST::Function const* func;
    Object*   result;
    Storage   storage;
    bool      isReturned;

    CallStack(AST::Function const* func)
      : func(func),
        result(nullptr),
        isReturned(false)
    {
    }
  };

  struct ScopeEvaluationFlags {
    //
    // isSkipped:
    //   if used "return" or "break" or "continue" --> true
    bool isSkipped = false;

    //
    // loop flags
    bool isBreaked    = false;
    bool isContinued  = false;
  };

public:

  Evaluator(AST::Scope* rootScope);

  /*
   * The core function.
   */
  Object* eval(AST::Base* ast);


  /*
   * Evaluate statements
   */
  void evalStatements(AST::Base* ast);


  /*
   * Evaluate a node as lvalue.
   * if can't be evaluated as lvalue, show error.
   */
  Object*& evalAsLeft(AST::Base* ast);


  /*
   * Evaluate operator in expression
   */
  Object* evalOperator(AST::Expr* expr);

private:

  Object*& evalIndexRef(AST::Expr* ast, Object* obj, Object* index);

  Object* evalCallFunc(AST::CallFunc* ast, Object* self, std::vector<Object*>& args);

  CallStack& push_stack(AST::Function const* func);
  void pop_stack();

  ScopeEvaluationFlags& getCurrentScope() {
    return *this->_scope;
  }

  bool inFunction() const {
    return !this->callStacks.empty();
  }

  CallStack& getCurrentCallStack() {
    return *callStacks.rbegin();
  }

  Storage& getCurrentStorage() {
    if( this->inFunction() )
      return this->getCurrentCallStack().storage;

    return this->globalStorage;
  }

  Object** findVariable(std::string_view name, bool allowCreate = true) {
    auto& storage = this->getCurrentStorage();

    if( !storage.contains(name) && !allowCreate )
      return nullptr;

    return &storage[name];
  }

  std::tuple<AST::Function const*, builtin::BuiltinFunc const*>
    findFunction(std::string_view name, Object* self);

  AST::Scope* rootScope;

  Storage  globalStorage;
  std::vector<CallStack> callStacks;

  ScopeEvaluationFlags* _loopScope;
  ScopeEvaluationFlags* _funcScope;
  ScopeEvaluationFlags* _scope;
};

} // namespace metro