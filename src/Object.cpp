#include <cassert>
#include <map>
#include "alert.h"
#include "GC.h"

namespace metro::objects {

None None::_none;

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

Object::Object(Type type)
  : type(std::move(type)),
    isMarked(false),
    noDelete(false)
{
  GC::_registerObject(this);
  alertmsg("obj new:    " << this);

  if( GC::isEnabled() )
    __dbg_map[this] = 1;
}

Object::~Object()
{
  alertmsg("obj delete: " << this);

  if( GC::isEnabled() )
    __dbg_map[this] = 0;
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

} // namespace metro::objects