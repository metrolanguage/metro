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
  };

  Kind kind;
  std::vector<Type> params;

  bool equals(Type const& type) const;
  std::string to_string() const;

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