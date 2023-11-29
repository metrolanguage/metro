#include <iostream>
#include <sstream>
#include <vector>
#include <cstdlib>

#include "AST.h"
#include "GC.h"
#include "BuiltinFunc.h"
#include "Error.h"

#define DEF(Name)       static Object* Name(AST::CallFunc* ast, std::vector<Object*>& args)
#define PASS(Name)      Name(ast, args)
#define BUILTIN(Name)   BuiltinFunc(#Name, Name)

#define MATCH(e...)     isMatch(args, e)

#define ILLEGAL \
  Error(ast->token).setMessage("illegal function call").emit().exit()

using namespace metro::objects;

namespace metro::builtin {

static bool _isMatch(std::vector<Object*> const& args, std::vector<Type> const& pattern) {
  if( args.size() != pattern.size() )
    return false;

  for( auto itObj = args.begin(); auto&& type : pattern ) {
    if( !type.equals((*itObj++)->type) ) {
      return false;
    }
  }

  return true;
}

template <class... Ts>
static bool isMatch(std::vector<Object*> const& args, Ts&&... ts) {
  return _isMatch(args, { std::forward<Ts>(ts)... });
}

//
// print
//
DEF( print ) {
  (void)ast;

  std::stringstream ss;

  for( auto&& arg : args )
    ss << arg->to_string();

  auto s = ss.str();

  std::cout << s;

  return new Int(s.length());
}

//
// println
//
DEF( println ) {
  (void)ast;

  args.emplace_back(new Char('\n'));
  return PASS(print);
}

//
// random
//
DEF( random ) {
  // range
  if( MATCH(Type::Range) ) {
    auto range = args[0]->as<Range>();
    return new Int(rand() % (range->end - range->begin));
  }
  // begin, end
  else if( MATCH(Type::Int, Type::Int) ) {
    auto begin = args[0]->as<Int>()->value;
    auto end = args[1]->as<Int>()->value;

    if( begin >= end ) {
      Error(ast->token)
        .setMessage("start value must less than end")
        .emit()
        .exit();
    }

    return new Int(rand() % (end - begin));
  }

  ILLEGAL;
}

//
// vector
//
DEF( vector ) {
  // range
  if( MATCH(Type::Range) ) {
    auto range = args[0]->as<Range>();
    auto vec = new Vector;

    for( auto i = range->begin; i < range->end; i++ ) {
      vec->elements.emplace_back(new Int(i));
    }

    return vec;
  }

  // count, object
  else if( MATCH(Type::USize, Type::Any) ) {
    auto vec = new Vector;

    for( size_t i = 0; i < args[0]->as<USize>()->value; i++ )
      vec->append(args[1]->clone());

    return vec;
  }

  ILLEGAL;
}

static std::vector<BuiltinFunc> const _all_functions {
  BUILTIN(print),
  BUILTIN(println),
  BUILTIN(random),
  BUILTIN(vector),

};

Object* BuiltinFunc::call(AST::CallFunc* ast, std::vector<Object*>& args) const {
  return this->impl(ast, args);
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