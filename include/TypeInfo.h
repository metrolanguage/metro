#pragma once

#include <string>
#include <vector>

namespace metro {

namespace AST {
  struct Enum;
  struct Struct;
}

struct TypeInfo {
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
    Any,    // ?
  };

  Kind kind;
  bool is_mutable;
  std::vector<TypeInfo> params;

  union {
    struct {
      AST::Enum* ast;
      size_t index;
    } enum_context;

    AST::Struct const* ast_struct;
  };

  bool equals(TypeInfo const& type) const;
  std::string to_string() const;

  bool is_iterable() const {
    switch( this->kind ) {
      case Kind::String:
      case Kind::Vector:
      case Kind::Dict:
      case Kind::Range:
        return true;
    }

    return false;
  }

  TypeInfo(Kind kind = None)
    : kind(kind),
      is_mutable(false),
      enum_context({nullptr, 0})
  {
  }

  TypeInfo(Kind kind, std::vector<TypeInfo>&& params)
    : TypeInfo(kind)
  {
    this->params = std::move(params);
  }
};

} // namespace metro