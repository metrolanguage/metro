/*----------------------------

  pseudo mark and sweep garbage collector.

  2023/11/22  (c) aoki

  ----------------------------*/

/* todo: translate all comments to English */

#include <map>
#include <thread>
#include <mutex>
#include <memory>
#include <algorithm>
#include "gc.h"
#include "alert.h"

//  the maximum count of object in memory.
//  注意：厳密には、この数に到達する手前になった段階で GC が作動します。
#define OBJECT_MEMORY_MAXIMUM   40

#define LOCK(...)  { std::lock_guard __lock(mtx); { __VA_ARGS__ } }

namespace metro::gc {

using namespace objects;

namespace {

bool isPaused;
bool isBusy;
std::mutex mtx;
std::thread* thread;
size_t mark_count;

std::vector<Object*> _Memory;
std::vector<Object*> root; /* binded objects for local-v */

std::vector<Object*>::iterator mFind(Object* object) {
  return std::find(root.begin(), root.end(), object);
}

void mAppend(Object* object) {
  for( auto&& p : _Memory )
    if( p == nullptr ) {
      p = object;
      return;
    }

  _Memory.emplace_back(object);
}

/* only delete a pointer from vector. don't call free or delete. */
void mDelete(Object* object) {
  *mFind(object) = nullptr;
}

void _Bind(Object* object) {
  if( std::find(root.cbegin(), root.cend(), object) == root.cend() ) {
    LOCK(
      root.emplace_back(object);
    )
  }
}

void _Unbind(Object* object) {
  if( auto it = std::find(root.cbegin(), root.cend(), object); it != root.cend() ) {
    LOCK(
      root.erase(it);
    )
  }
}

void _Mark(Object* object) {
  object->isMarked = true;
  mark_count++;

  switch( object->type.kind ) {
    case Type::Vector: {
      auto vec = object->as<Vector>();

      for( auto&& e : vec->elements )
        _Mark(e);

      break;
    }

    case Type::Dict: {
      auto vec = object->as<Dict>();

      for( auto&& [k, v] : vec->elements ) {
        _Mark(k);
        _Mark(v);
      }

      break;
    }
  }
}

void _Unmark() {
  for( auto&& p : _Memory )
    if( p ) p->isMarked = false;
}

void _MarkAll() {
  mark_count = 0;

  for( auto&& r : root )
    _Mark(r);

  if( mark_count >= OBJECT_MEMORY_MAXIMUM ) {
    printf("metro: out of memory.\n");
    std::exit(1);
  }
}

void _CollectThreadFunc() {
  _Unmark();
  _MarkAll();

  for( auto&& pObj : _Memory )
    if( pObj && !pObj->isMarked ) {
      delete pObj;
      pObj = nullptr;
    }

  isBusy = false;
}

void _Collect() {
  if( isBusy )
    return;

  if( thread ) {
    thread->join();
    delete thread;
  }

  isBusy = true;
  thread = new std::thread(_CollectThreadFunc);
}

}

ObjectBinder::ObjectBinder(Object* obj)
  : object(obj)
{
  _Bind(obj);
}

ObjectBinder::~ObjectBinder()
{
  _Unbind(this->object);
}

void ObjectBinder::reset(Object* obj) {
  _Unbind(this->object);
  _Bind(this->object = obj);
}

Object* ObjectBinder::get() const {
  return this->object;
}

void pause() {
  isPaused = true;
}

void resume() {
  isPaused = false;
}

void addObject(objects::Object* object) {
  if( !isPaused && _Memory.size() >= OBJECT_MEMORY_MAXIMUM ) {
    _Collect();
  }

  mAppend(object);
}

ObjectBinder make_binder(objects::Object* object) {
  return ObjectBinder(object);
}

void bind(objects::Object* obj) {
  _Bind(obj);
}

void unbind(objects::Object* obj) {
  _Unbind(obj);
}

void doCollectForce() {
  _Collect();
}

void clean() {
  for( auto&& obj : _Memory )
    if( obj ) {
      delete obj;
    }

  _Memory.clear();
  root.clear();

  if( thread ) {
    thread->join();
    delete thread;
  }
}

objects::Object* cloneObject(objects::Object* obj) {
  switch( obj->type.kind ) {
    case Type::None:
      return newObject<None>();
    
    case Type::Int:
      return newObject<Int>(obj->as<Int>()->value);
    
    case Type::Float:
      return newObject<Float>(obj->as<Float>()->value);

    case Type::USize:
      return newObject<USize>(obj->as<USize>()->value);
    
    case Type::Bool:
      return newObject<Bool>(obj->as<Bool>()->value);
    
    case Type::Char:
      return newObject<Char>(obj->as<Char>()->value);
    
    case Type::String:
      return newObject<String>(obj->as<String>()->value);

    case Type::Vector: {
      auto x = newObject<Vector>();

      for( auto&& e : obj->as<Vector>()->elements )
        x->append(cloneObject(e));

      return x;
    }

    case Type::Dict: {
      auto x = newObject<Dict>();

      for( auto&& [k, v] : obj->as<Dict>()->elements )
        x->append(cloneObject(k), cloneObject(v));

      return x;
    }
  }

  alert;
  return nullptr;
}

} // namespace metro::gc