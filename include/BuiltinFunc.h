#pragma once

#include <functional>
#include "Object.h"
#include "AST.h"

namespace metro::builtin {

/*
 * a builtin function for metro.
 */

struct BuiltinFunc {
  using Object = objects::Object;

  using FuncPointer =
    std::function<Object*(AST::CallFunc const*, std::vector<Object*> const&)>;

  std::string   name;
  bool          have_self;
  Type          self_type;
  FuncPointer   impl;

  Object* call(AST::CallFunc const* ast, std::vector<Object*> const& args) const {
    return this->impl(ast, args);
  }

  static BuiltinFunc const* find(std::string const& name);
  static std::vector<BuiltinFunc> const& getAllFunctions();

  BuiltinFunc(std::string const& name, FuncPointer impl, bool haveSelf, Type&& selfType)
    : name(name),
      have_self(haveSelf),
      self_type(std::move(selfType)),
      impl(impl)
  {
  }
};

} // namespace metro::builtin