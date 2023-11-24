#pragma once 

#include <codecvt>
#include <limits>
#include <locale>
#include "Type.h"

namespace metro::objects {

template <class T, Type::Kind k>
struct _Primitive;

using Int     = _Primitive<int64_t,        Type::Int>;
using Float   = _Primitive<double,         Type::Float>;
using Bool    = _Primitive<bool,           Type::Bool>;
using USize   = _Primitive<size_t,         Type::USize>;
using Char    = _Primitive<char16_t,       Type::Char>;
using String  = _Primitive<std::u16string, Type::String>;

struct Object {
  Type type;
  bool isMarked;

  template <class T>
  T* as() const {
    return (T*)this;
  }

  virtual std::string to_string() const = 0;
  virtual Object* clone() const = 0;

  virtual ~Object() { }

protected:
  Object(Type type);
};

struct None : Object {
  std::string to_string() const {
    return "none";
  }

  None* clone() const {
    return new None;
  }

  static None* getNone() {
    return &_none;
  }

  None()
    : Object(Type::None)
  {
  }

private:
  static None _none;
};

template <class T, Type::Kind k>
struct _Primitive : Object {
  T value;

  std::string to_string() const {
    return std::to_string(this->value);
  }

  _Primitive* clone() const {
    return new _Primitive<T, k>(this->value);
  }

  _Primitive(T val = T{ })
    : Object(k),
      value(val)
  {
  }
};

template <>
struct _Primitive<std::u16string, Type::String> : Object {
  std::u16string value;

  std::string to_string() const {
    return conv.to_bytes(this->value);
  }

  String* clone() const {
    return new String(this->value);
  }

  _Primitive(std::u16string const& val = u"")
    : Object(Type::String),
      value(val)
  {
  }

  _Primitive(std::string const& str)
    : Object(Type::String),
      value(conv.from_bytes(str))
  {
  }

private:
  static inline std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;
  friend struct _Primitive<char16_t, Type::Char>;
};

template <>
struct _Primitive<char16_t, Type::Char> : Object {
  char16_t value;

  std::string to_string() const {
    return String::conv.to_bytes(std::u16string(1, this->value));
  }

  Char* clone() const {
    return new Char(this->value);
  }

  _Primitive(char16_t val = 0)
    : Object(Type::Char),
      value(val)
  {
  }
};

struct Vector : Object {
  std::vector<Object*> elements;

  auto begin() const { return this->elements.begin(); }
  auto end() const { return this->elements.end(); }

  auto& append(Object* obj) {
    return this->elements.emplace_back(obj);
  }

  std::string to_string() const {
    if( this->elements.empty() )
      return "[]";
    
    std::string ret = "[";

    for( auto&& e : this->elements ) {
      ret += e->to_string();
      if( e != *this->elements.rbegin() ) ret += ", ";
    }

    return ret + ']';
  }

  Vector* clone() const {
    auto vec = new Vector();

    for( auto&& e : this->elements )
      vec->append(e->clone());
    
    return vec;
  }

  Vector(std::vector<Object*> elements = { })
    : Object(Type::Vector),
      elements(std::move(elements))
  {
  }
};

struct Dict : Object {
  std::vector<std::pair<Object*, Object*>> elements;

  auto begin() const { return this->elements.begin(); }
  auto end() const { return this->elements.end(); }

  auto& append(Object* key, Object* value) {
    return this->elements.emplace_back(key, value);
  }

  std::string to_string() const {
    if( this->elements.empty() )
      return "{}";
    
    std::string ret = "{";

    for( auto it = this->begin(); it != this->end(); it++ ){
      ret += it->first->to_string() + ": " + it->second->to_string();
      if( *it == *this->elements.rbegin() ) ret += ", ";
    }

    return ret + '}';
  }

  Dict* clone() const {
    auto dict = new Dict();

    for( auto&& [k, v] : this->elements )
      dict->append(k->clone(), v->clone());

    return dict;
  }

  Dict(std::vector<std::pair<Object*, Object*>>&& elements = { })
    : Object(Type::Dict),
      elements(std::move(elements))
  {
  }
};

} // namespace metro::objects

