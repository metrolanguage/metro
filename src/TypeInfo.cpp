#include "TypeInfo.h"
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
  "",   // struct
  "",   // enum
  "args",
  "any",
};

//
// 等しいかどうか評価
bool TypeInfo::equals(TypeInfo const& type) const {
  if( this->kind == TypeInfo::Any || type.kind == TypeInfo::Any )
    return true;

  if( this->kind != type.kind )
    return false;

  if( this->kind == TypeInfo::Struct && this->ast_struct != type.ast_struct )
    return false;

  //
  // enum
  if( this->kind == TypeInfo::Enumerator
    && (this->enum_context.ast != type.enum_context.ast || this->enum_context.index != type.enum_context.index) )
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

std::string TypeInfo::to_string() const {
  switch( this->kind ) {
    case TypeInfo::Enumerator:
      return
        this->enum_context.ast->getName()
          + "." + std::string(this->enum_context.ast->enumerators[this->enum_context.index]->str );

    case TypeInfo::Struct:
      return this->ast_struct->getName();
  }

  std::string ret = names[static_cast<int>(this->kind)];

  if( !this->params.empty() ) {
    ret += "<";

    for( auto&& p : this->params ) {
      ret += p.to_string();
      if( &p != &*this->params.rbegin() ) ret += ", ";
    }

    ret += ">";
  }

  return ret;
}

} // namespace metro