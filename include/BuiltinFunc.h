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
  using FuncPointer = std::function<Object*(AST::CallFunc*, std::vector<Object*>&)>;

  std::string   name;
  bool          have_self;
  TypeInfo      self_type;
  FuncPointer   impl;

  Object* call(AST::CallFunc* ast, std::vector<Object*>& args) const;

  static BuiltinFunc const* find(std::string const& name);
  static std::vector<BuiltinFunc> const& getAllFunctions();

  BuiltinFunc(std::string const& name, FuncPointer impl)
    : name(name),
      have_self(false),
      impl(impl)
  {
  }
};

} // namespace metro::builtin