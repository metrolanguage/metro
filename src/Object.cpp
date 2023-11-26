#include "GC.h"

namespace metro::objects {

None None::_none;

Object::Object(Type type)
  : type(std::move(type)),
    isMarked(false)
{
  GC::_registerObject(this);
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

} // namespace metro::objects