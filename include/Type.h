#pragma once

#include <string>
#include <vector>

namespace metro {

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
    Range,
  };

  Kind kind;
  std::vector<Type> params;

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
    : kind(kind)
  {
  }

  Type(Kind kind, std::vector<Type>&& params)
    : kind(kind),
      params(std::move(params))
  {
  }
};

} // namespace metro