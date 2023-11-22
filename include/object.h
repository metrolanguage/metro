#pragma once 

#include <codecvt>
#include <limits>
#include <locale>
#include "type.h"

namespace metro::objects {

template <class T, Type::Kind k>
struct _Primitive;

using Int     = _Primitive<int64_t,        Type::Int>;
using Float   = _Primitive<double,         Type::Float>;
using Bool    = _Primitive<bool,           Type::Bool>;
using USize   = _Primitive<size_t,         Type::USize>;
using Char    = _Primitive<char16_t,       Type::Char>;
using String  = _Primitive<std::u16string, Type::String>;

struct Base {
  Type type;
  bool isMarked;

  template <class T>
  T* as() const {
    return (T*)this;
  }

  virtual std::string to_string() const = 0;

  virtual ~Base() { }

protected:
  Base(Type type)
    : type(std::move(type)),
      isMarked(false)
  {
  }
};

template <class T, Type::Kind k>
struct _Primitive : Base {
  T value;

  std::string to_string() const {
    return std::to_string(this->value);
  }

  _Primitive(T val = T{ })
    : Base(k),
      value(val)
  {
  }
};

template <>
struct _Primitive<std::u16string, Type::String> : Base {
  std::u16string value;

  std::string to_string() const {
    return conv.to_bytes(this->value);
  }

  _Primitive(std::u16string const& val = u"")
    : Base(Type::String),
      value(val)
  {
  }

private:
  static inline std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;
  friend struct _Primitive<char16_t, Type::Char>;
};

template <>
struct _Primitive<char16_t, Type::Char> : Base {
  char16_t value;

  std::string to_string() const {
    return String(std::u16string(1, this->value)).to_string();
  }

  _Primitive(char16_t val = 0)
    : Base(Type::Char),
      value(val)
  {
  }
};

struct Vector : Base {
  std::vector<Base*> elements;

  auto begin() const { return this->elements.begin(); }
  auto end() const { return this->elements.end(); }

  auto& append(Base* obj) {
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

  Vector(std::vector<Base*> elements = { })
    : Base(Type::Vector),
      elements(std::move(elements))
  {
  }
};

struct Dict : Base {
  std::vector<std::pair<Base*, Base*>> elements;

  auto begin() const { return this->elements.begin(); }
  auto end() const { return this->elements.end(); }

  auto& append(Base* key, Base* value) {
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

  Dict(std::vector<std::pair<Base*, Base*>>&& elements = { })
    : Base(Type::Dict),
      elements(std::move(elements))
  {
  }
};

} // namespace metro::objects

