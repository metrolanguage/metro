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