#pragma once

#include <functional>
#include "object.h"

namespace metro::builtin {

/*
 * a builtin function for metro.
 *
 */

struct BuiltinFunc {
  using Object = objects::Object;
  using Implementation = std::function<Object*(std::vector<Object*>)>;

  std::string     name;
  int             arg_count; // -1 = free args
  Implementation  impl;

  Object* call(std::vector<Object*> args) const;

  static BuiltinFunc const* find(std::string const& name);
  static std::vector<BuiltinFunc> const& getAllFunctions();

  BuiltinFunc(std::string const& name, int arg_count, Implementation impl)
    : name(name),
      arg_count(arg_count),
      impl(impl)
  {
  }
};

} // namespace metro::builtin