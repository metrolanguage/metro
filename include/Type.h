#pragma once

#include <string>
#include <vector>

namespace metro {

namespace AST {
  struct Enum;
  struct Struct;
}

struct Type {
  enum Kind {
    None,
    Int,
    Float,
    USize,
    Bool,
    Char,
    String,
    Vector,
    Dict,
    Tuple,
    Pair,
    Range,
    Struct,
    Enumerator,
    Args,
    Any,
  };

  Kind kind;
  bool isConst;
  std::vector<Type> params;

  union {
    struct {
      AST::Enum const* astEnum;
      size_t enumeratorIndex;
    };

    AST::Struct const* astStruct;
  };

  bool equals(Type const& type) const;
  std::string toString() const;

  bool isIterable() const {
    switch( this->kind ) {
      case Kind::String:
      case Kind::Vector:
      case Kind::Dict:
      case Kind::Range:
        return true;
    }

    return false;
  }

  Type(Kind kind = None)
    : kind(kind),
      isConst(false),
      astEnum(nullptr),
      enumeratorIndex(0)
  {
  }

  Type(Kind kind, std::vector<Type>&& params)
    : Type(kind)
  {
    this->params = std::move(params);
  }
};

} // namespace metro