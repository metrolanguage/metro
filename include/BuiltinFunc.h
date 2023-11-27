#pragma once

#include <functional>
#include "Object.h"

namespace metro::builtin {

/*
 * a builtin function for metro.
 */

struct BuiltinFunc {
  using Object = objects::Object;
  using FuncPointer = std::function<Object*(std::vector<Object*>)>;

  std::string   name;
  int           arg_count; // -1 = free args
  FuncPointer   impl;

  Object* call(std::vector<Object*> args) const;

  static BuiltinFunc const* find(std::string const& name);
  static std::vector<BuiltinFunc> const& getAllFunctions();

  BuiltinFunc(std::string const& name, int arg_count, FuncPointer impl)
    : name(name),
      arg_count(arg_count),
      impl(impl)
  {
  }
};

} // namespace metro::builtin