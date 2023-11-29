#include "Type.h"
#include "AST.h"

namespace metro {

static char const* names[] {
  "none",
  "int",
  "float",
  "usize",
  "bool",
  "char",
  "string",
  "vector",
  "dict",
  "tuple",
  "range",
  "",
  "",
  "args",
  "any",
};

bool Type::equals(Type const& type) const {
  if( this->kind == Type::Any || type.kind == Type::Any )
    return true;

  if( this->kind != type.kind )
    return false;

  if( this->kind == Type::Struct && this->astStruct != type.astStruct )
    return false;

  if( this->kind == Type::Enumerator
    && (this->astEnum != type.astEnum || this->enumeratorIndex != type.enumeratorIndex) )
    return false;

  if( this->params.size() != type.params.size() )
    return false;

  if( !this->params.empty() ) {
    for( auto it = type.params.begin(); auto&& p : this->params )
      if( !it++->equals(p) )
        return false;
  }

  return true;
}

std::string Type::toString() const {
  switch( this->kind ) {
    case Type::Enumerator:
      return
        this->astEnum->getName()
          + "." + std::string(this->astEnum->enumerators[this->enumeratorIndex]->str );

    case Type::Struct:
      return this->astStruct->getName();
  }

  std::string ret = names[static_cast<int>(this->kind)];

  if( !this->params.empty() ) {
    ret += "<";

    for( auto&& p : this->params ) {
      ret += p.toString();
      if( &p != &*this->params.rbegin() ) ret += ", ";
    }

    ret += ">";
  }

  return ret;
}

} // namespace metro