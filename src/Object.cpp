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
  if( !this->type.equals(object->type) )
    return false;
  
  switch( this->type.kind ) {
    case Type::Int:
      return this->as<Int>()->equals(object->as<Int>());
      
    case Type::Float:
      return this->as<Float>()->equals(object->as<Float>());
      
    case Type::USize:
      return this->as<USize>()->equals(object->as<USize>());
      
    case Type::Bool:
      return this->as<Bool>()->equals(object->as<Bool>());
      
    case Type::Char:
      return this->as<Char>()->equals(object->as<Char>());

    case Type::String:
      return this->as<String>()->equals(object->as<String>());

    case Type::Vector:
      return this->as<Vector>()->equals(object->as<Vector>());

    case Type::Dict:
      return this->as<Dict>()->equals(object->as<Dict>());

    case Type::Tuple:
      return this->as<Tuple>()->equals(object->as<Tuple>());

    case Type::Range:
      return this->as<Range>()->equals(object->as<Range>());
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