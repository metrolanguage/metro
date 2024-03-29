#pragma once 

#include <codecvt>
#include <locale>
#include "TypeInfo.h"
#include "Utils.h"

namespace metro::objects {

template <class T, TypeInfo::Kind k>
struct _Primitive;

using Int     = _Primitive<int64_t,        TypeInfo::Int>;
using Float   = _Primitive<double,         TypeInfo::Float>;
using Bool    = _Primitive<bool,           TypeInfo::Bool>;
using USize   = _Primitive<size_t,         TypeInfo::USize>;
using Char    = _Primitive<char16_t,       TypeInfo::Char>;
using String  = _Primitive<std::u16string, TypeInfo::String>;

struct Object {
  TypeInfo type;
  bool isMarked;
  bool noDelete;

  template <class T>
  T* as() const {
    return (T*)this;
  }

  bool isNumeric() const;
  bool equals(Object* object) const;

  virtual std::string to_string() const = 0;
  virtual Object* clone() const = 0;

  virtual ~Object();

protected:
  Object(TypeInfo type);
};

struct None : Object {
  std::string to_string() const {
    return "none";
  }

  None* clone() const {
    return new None;
  }

  bool equals(None*) const {
    return true;
  }

  static None* getNone() {
    return &_none;
  }

  None()
    : Object(TypeInfo::None)
  {
  }

private:
  static None _none;
};

template <class T, TypeInfo::Kind k>
struct _Primitive : Object {
  using ValueType = T;
  
  T value;

  std::string to_string() const {
    return std::to_string(this->value);
  }

  _Primitive* clone() const {
    return new _Primitive<T, k>(this->value);
  }

  bool equals(_Primitive* obj) const {
    return this->value == obj->value;
  }

  _Primitive(T val = T{ })
    : Object(k),
      value(val)
  {
  }
};

template <>
struct _Primitive<bool, TypeInfo::Bool> : Object {
  bool value;

  std::string to_string() const {
    return this->value ? "true" : "false";
  }

  Bool* clone() const {
    return new Bool(this->value);
  }

  bool equals(Bool* obj) const {
    return this->value == obj->value;
  }

  _Primitive(bool val = false)
    : Object(TypeInfo::Bool),
      value(val)
  {
  }
};

template <>
struct _Primitive<std::u16string, TypeInfo::String> : Object {
  std::vector<Char*> value;

  std::string to_string() const;
  String* clone() const;

  bool equals(String*) const;

  _Primitive(std::u16string const& str = u"");
  _Primitive(std::string const& str);
  _Primitive(std::vector<Char*> val);

  String* append(Char* ch);
  String* append(String* str);

  static inline
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;
};

template <>
struct _Primitive<char16_t, TypeInfo::Char> : Object {
  char16_t value;

  std::string to_string() const {
    return String::conv.to_bytes(std::u16string(1, this->value));
  }

  Char* clone() const {
    return new Char(this->value);
  }

  bool equals(Char* c) const {
    return this->value == c->value;
  }

  _Primitive(char16_t val = 0)
    : Object(TypeInfo::Char),
      value(val)
  {
  }
};

//
// Vector
//  --> vector, struct
//
struct Vector : Object {
  std::vector<Object*> elements;

  auto begin() const { return this->elements.begin(); }
  auto end() const { return this->elements.end(); }

  auto& append(Object* obj) {
    return this->elements.emplace_back(obj);
  }

  std::string to_string() const;
  Vector* clone() const;

  bool equals(Vector* vec) const;

  /*
   * getMember():
   *   find the member with name
   *   only can use in TypeInfo::Struct
   */
  Object** getMember(std::string const& name);

  Vector(std::vector<Object*> elements = { })
    : Object(TypeInfo::Vector),
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

  bool equals(Dict* dict) const {
    if( this->elements.size() != dict->elements.size() )
      return false;

    for( auto it = dict->elements.begin(); auto&& [k, v] : this->elements ) {
      auto&& [k2, v2] = *it++;

      if( !k->equals(k2) || !v->equals(v2) )
        return false;
    }

    return true;
  }

  Dict(std::vector<std::pair<Object*, Object*>>&& elements = { })
    : Object(TypeInfo::Dict),
      elements(std::move(elements))
  {
  }
};

struct Tuple : Object {
  std::vector<Object*> elements;

  std::string to_string() const {
    std::string ret = "(";

    for( auto&& e : this->elements ) {
      ret += e->to_string();
      if( e != *this->elements.rbegin() ) ret += ", ";
    }

    return ret + ')';
  }

  Tuple* clone() const {
    auto tuple = new Tuple({ });

    for( auto&& e : this->elements )
      tuple->elements.emplace_back(e->clone());
    
    return tuple;
  }

  bool equals(Tuple* tu) const {
    if( this->elements.size() != tu->elements.size() )
      return false;

    for( auto it = this->elements.begin(); auto&& e : tu->elements )
      if( !(*it)->equals(e) )
        return false;

    return true;
  }

  Tuple(std::vector<Object*> elements)
    : Object(TypeInfo::Tuple),
      elements(std::move(elements))
  {
  }
};

struct Pair : Object {
  Object* first;
  Object* second;

  std::string to_string() const {
    return first->to_string() + ": " + second->to_string();
  }

  Pair* clone() const {
    return new Pair(this->first->clone(), this->second->clone());
  }

  bool equals(Pair* pair) const {
    return
      this->first->equals(pair->first)
      && this->second->equals(pair->second);
  }

  Pair(Object* first, Object* second)
    : Object(TypeInfo::Pair),
      first(first),
      second(second)
  {
  }
};

struct Range : Object {
  int64_t begin;
  int64_t end;

  std::string to_string() const {
    return utils::format("%llu .. %llu", this->begin, this->end);
  }

  Range* clone() const {
    return new Range(this->begin, this->end);
  }

  bool equals(Range* range) const {
    return this->begin == range->begin && this->end == range->end;
  }

  Range(int64_t begin, int64_t end)
    : Object(TypeInfo::Range),
      begin(begin),
      end(end)
  {
  }
};

} // namespace metro::objects

