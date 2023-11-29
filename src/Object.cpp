#include <cassert>
#include <map>
#include "alert.h"
#include "GC.h"
#include "AST.h"

namespace metro::objects {

None None::_none;

debug(
  std::map<Object*, bool> __dbg_map;

  struct XX {
    ~XX() {
      for( auto&& [p, b] : __dbg_map ) {
        if( b ) {
          alertmsg("memleak: " << p << ": " << p->to_string());
        }
      }
    }
  };

  static XX _xx;
)

Object::Object(Type type)
  : type(std::move(type)),
    isMarked(false),
    noDelete(false)
{
  GC::_registerObject(this);

  debug(
    if( GC::isEnabled() )
      __dbg_map[this] = 1;
  )
}

Object::~Object()
{
  debug(
    if( GC::isEnabled() )
      __dbg_map[this] = 0;
  )
}

bool Object::isNumeric() const {
  switch( type.kind ) {
    case Type::Int:
    case Type::Float:
    case Type::USize:
      return true;
  }

  return false;
}

bool Object::equals(Object* object) const {
#define __CASE__(T) \
  case Type::T: return this->as<T>()->equals(object->as<T>());

  if( !this->type.equals(object->type) )
    return false;

  switch( this->type.kind ) {
    __CASE__(Int)
    __CASE__(Float)
    __CASE__(USize)
    __CASE__(Bool)
    __CASE__(Char)
    __CASE__(String)
    __CASE__(Vector)
    __CASE__(Dict)
    __CASE__(Tuple)
    __CASE__(Range)
  }

  assert(this->type.equals(Type::None));

  return true;
}

std::string _Primitive<std::u16string, Type::String>::to_string() const {
  std::u16string s;

  for( auto&& c : this->value )
    s += c->value;
  
  return conv.to_bytes(s);
}

String* _Primitive<std::u16string, Type::String>::clone() const {
  return new String(this->value);
}

bool String::equals(String* str) const {
  if( this->value.size() != str->value.size() )
    return false;

  for( auto it = this->value.begin(); auto&& c : str->value )
    if( !(*it++)->equals(c) )
      return false;

  return true;
}

_Primitive<std::u16string, Type::String>::_Primitive(std::u16string const& str)
  : Object(Type::String)
{
  for( auto&& c : str )
    this->value.emplace_back(new Char(c));
}

_Primitive<std::u16string, Type::String>::_Primitive(std::string const& str)
  : _Primitive(conv.from_bytes(str))
{
}

_Primitive<std::u16string, Type::String>::_Primitive(std::vector<Char*> val)
  : Object(Type::String),
    value(std::move(val))
{
}

String* String::append(Char* ch) {
  this->value.emplace_back(ch);
  return this;
}

String* String::append(String* str) {
  for( auto&& c : str->value )
    this->append(c->clone());

  return this;
}

std::string Vector::to_string() const {
  if( this->elements.empty() )
    return "[]";
  
  std::string ret;

  auto isStruct = this->type.kind == Type::Struct;

  for( size_t i = 0; i < this->elements.size(); i++ ) {
    auto& e = this->elements[i];

    if( isStruct )
      ret += std::string(this->type.astStruct->members[i]->str) + ": ";

    ret += e->to_string();

    if( e != *this->elements.rbegin() )
      ret += ", ";
  }

  if( isStruct )
    return this->type.astStruct->getName() + "{" + ret + "}";

  return '[' + ret + ']';
}

Vector* Vector::clone() const {
  auto vec = new Vector();

  vec->type = this->type;

  for( auto&& e : this->elements )
    vec->append(e->clone());

  return vec;
}

bool Vector::equals(Vector* vec) const {
  if( this->elements.size() != vec->elements.size() )
    return false;

  for( auto it = this->elements.begin(); auto&& e : vec->elements )
    if( !(*it)->equals(e) )
      return false;

  return true;
}

/*
  * getMember():
  *   find the member with name
  *   only can use in Type::Struct
  */
Object** Vector::getMember(std::string const& name) {
  for( size_t i = 0; auto&& m : this->type.astStruct->members ) {
    if( m->str == name )
      return &this->elements[i];

    i++;
  }

  return nullptr;
}

} // namespace metro::objects