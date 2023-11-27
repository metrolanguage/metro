/*----------------------------

  pseudo mark and sweep garbage collector.

  2023/11/22  (c) aoki

  ----------------------------*/

#include <map>
#include <thread>
#include <mutex>
#include <memory>
#include <algorithm>
#include "GC.h"
#include "alert.h"
#include "metro.h"

//  The maximum count of object in memory.
//  Collect all objects when overed this count.
#define OBJECT_MEMORY_MAXIMUM   (1 << 14)

#define LOCK(...)  { std::lock_guard __lock(mtx); { __VA_ARGS__ } }

namespace metro::GC {

using namespace objects;

namespace {

bool _isEnabled;
bool _isBusy;
std::mutex mtx;
std::thread* thread;
size_t mark_count;

std::vector<Object*> _Memory;
std::vector<Object*> _MemorySub;
std::vector<Object*> root; /* binded objects for local-v */

Object* mAppend(Object* object) {
  std::lock_guard Lock(mtx);

  for( auto&& p : _Memory ) {
    if( p == nullptr )
      return p = object;
  }

  return _Memory.emplace_back(object);
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

void _MarkAll() {
  mark_count = 0;

  for( auto&& r : root )
    _Mark(r);

  if( mark_count >= OBJECT_MEMORY_MAXIMUM ) {
    Metro::getInstance()->fatalError("metro: out of memory.\n");
  }
}

void _CollectThreadFunc() {
  _MarkAll();

  LOCK(
    _MemorySub = _Memory;
    _Memory.clear();
  )

  for( auto it = _MemorySub.begin(); it != _MemorySub.end(); ) {
    if( *it && !(*it)->noDelete && !(*it)->isMarked ) {
      delete *it;

      _MemorySub.erase(it);

      continue;
    }
    else {
      (*it++)->isMarked = false;
    }
  }

  LOCK(
    for( auto&& obj : _Memory ) {
      _MemorySub.emplace_back(obj);
    }

    _Memory = std::move(_MemorySub);
  )

  _isBusy = false;
}

void _Collect() {

  if( _isBusy )
    return;

  if( thread ) {
    thread->join();
    delete thread;
  }


  _isBusy = true;
  thread = new std::thread(_CollectThreadFunc);

}

} // unonymous

void initialize() {
  _isEnabled = true;

  _Memory.reserve(OBJECT_MEMORY_MAXIMUM);
  _MemorySub.reserve(OBJECT_MEMORY_MAXIMUM);
}

bool isEnabled() {
  return _isEnabled;
}

Object* _registerObject(Object* object) {
  if( !isEnabled() )
    return object;

  if( _Memory.size() >= OBJECT_MEMORY_MAXIMUM ) {
    _Collect();
  }

  return mAppend(object);
}

void bind(Object* obj) {
  if( !isEnabled() )
    return;

  _Bind(obj);
}

void unbind(Object* obj) {
  if( !isEnabled() )
    return;

  _Unbind(obj);
}

void doCollectForce() {
  if( !isEnabled() )
    return;

  _Collect();
}

void exitGC() {
  if( !isEnabled() )
    return;

  if( thread ) {
    thread->join();
    delete thread;
  }

  for( auto&& obj : _Memory ) {
    if( obj ) {
      delete obj;
    }
  }

  _Memory.clear();
  root.clear();
}

} // namespace metro::GC