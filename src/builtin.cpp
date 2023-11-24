#include <iostream>
#include <sstream>
#include <vector>
#include "BuiltinFunc.h"
#include "gc.h"

#define DEF(name, argc) \
  BuiltinFunc(name, argc, [] (std::vector<Object*> args) -> Object* {

#define ENDEF \
  }),

using namespace metro::objects;

namespace metro::builtin {

static std::vector<BuiltinFunc> const _all_functions {
  DEF("print", -1)
    std::stringstream ss;

    for( auto&& arg : args )
      ss << arg->to_string();

    auto str = ss.str();

    std::cout << str;

    return gc::newObject<Int>((int)str.length());
  ENDEF

  DEF("println", -1)
    std::stringstream ss;

    for( auto&& arg : args )
      ss << arg->to_string();

    auto str = ss.str();

    std::cout << str << std::endl;

    return gc::newObject<Int>((int)str.length() + 1);
  ENDEF
};

Object* BuiltinFunc::call(std::vector<Object*> args) const {
  return this->impl(std::move(args));
}

BuiltinFunc const* BuiltinFunc::find(std::string const& name) {
  for( auto&& func : _all_functions )
    if( func.name == name )
      return &func;

  return nullptr;
}

std::vector<BuiltinFunc> const& BuiltinFunc::getAllFunctions() {
  return _all_functions;
}

} // namespace metro::builtin