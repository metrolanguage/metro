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
    refCount(0),
    _isUndead(false)
{
  GC::_registerObject(this);

  debug(
    if( GC::isEnabled() ) __dbg_map[this] = 1;
  )
}

Object::~Object()
{
  debug(
    if( GC::isEnabled() ) __dbg_map[this] = 0;
  )
}

Object* Object::to_undead() {
  this->_isUndead = true;

  switch( this->type.kind ) {
    case Type::String:
      for( auto&& ch : this->as<String>()->characters )
        ch->to_undead();

      break;

    case Type::Vector:
      for( auto&& elem : this->as<Vector>()->elements )
        elem->to_undead();

      break;

    case Type::Dict:
      for( auto&& [key, value] : this->as<Dict>()->elements ) {
        key->to_undead();
        value->to_undead();
      }

      break;

    case Type::Tuple:
      for( auto&& elem : this->as<Tuple>()->elements )
        elem->to_undead();

      break;

    case Type::Pair: {
      auto pair = this->as<Pair>();

      pair->first->to_undead();
      pair->second->to_undead();

      break;
    }

    case Type::Range: {
      
    }
  }

  return this;
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

template <>
std::string _Primitive<double, Type::Float>::to_string() const {
  auto str = std::to_string(this->value);

  while( str.length() > 0 && *str.rbegin() == '0' )
    str.pop_back();

  return str;
}

//
// String::to_string()
//
std::string _Primitive<std::u16string, Type::String>::to_string() const {
  std::u16string s;

  for( auto&& c : this->characters )
    s += c->value;
  
  return conv.to_bytes(s);
}

//
// String::clone()
//
String* _Primitive<std::u16string, Type::String>::clone() const {
  return new String(this->characters);
}

bool String::equals(String* str) const {
  if( this->characters.size() != str->characters.size() )
    return false;

  for( auto it = this->characters.begin(); auto&& c : str->characters )
    if( !(*it++)->equals(c) )
      return false;

  return true;
}

_Primitive<std::u16string, Type::String>::_Primitive(std::u16string const& str)
  : Object(Type::String)
{
  for( auto&& c : str )
    this->characters.emplace_back(new Char(c));
}

_Primitive<std::u16string, Type::String>::_Primitive(std::string const& str)
  : _Primitive(conv.from_bytes(str))
{
}

_Primitive<std::u16string, Type::String>::_Primitive(std::vector<Char*> characters)
  : Object(Type::String),
    characters(std::move(characters))
{
}

String* String::append(Char* ch) {
  this->characters.emplace_back(ch);
  return this;
}

String* String::append(String* str) {
  for( auto&& c : str->characters )
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