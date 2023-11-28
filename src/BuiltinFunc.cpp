#include <iostream>
#include <sstream>
#include <vector>
#include <cstdlib>

#include "AST.h"
#include "GC.h"
#include "BuiltinFunc.h"
#include "Error.h"

#define DEF(name, argc, code) \
  BuiltinFunc(name, argc, \
    [] (AST::CallFunc* ast, std::vector<Object*> args) -> Object* {(void)(ast, args); code})

using namespace metro::objects;

namespace metro::builtin {

static std::vector<BuiltinFunc> const _all_functions {
  DEF("print", -1, {
    std::stringstream ss;

    for( auto&& arg : args )
      ss << arg->to_string();

    auto str = ss.str();

    std::cout << str;

    return new Int((int)str.length());
  }),

  DEF("println", -1, {
    std::stringstream ss;

    for( auto&& arg : args )
      ss << arg->to_string();

    auto str = ss.str();

    std::cout << str << std::endl;

    return new Int((int)str.length() + 1);
  }),

  DEF("random", -1, {
    if( args.size() == 1 ) { // range
      if( !args[0]->type.equals(Type::Range) ) {
        goto invalid_args;
      }

      auto range = args[0]->as<Range>();
      return new Int(rand() % (range->end - range->begin));
    }

  invalid_args:
    Error(ast)
      .setMessage("invalid arguments").emit().exit();
  }),
};

Object* BuiltinFunc::call(AST::CallFunc* ast, std::vector<Object*> args) const {
  return this->impl(ast, std::move(args));
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