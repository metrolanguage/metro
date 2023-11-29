/*----------------------------

  pseudo mark and sweep garbage collector.

  2023/11/22  (c) aoki

  ----------------------------*/

#include <cassert>
#include <map>
#include <thread>
#include <mutex>
#include <memory>
#include <algorithm>

#include "alert.h"
#include "Metro.h"
#include "GC.h"

//  first maximum count of object in memory.
//  Collect all objects when overed this count.
#define OBJECT_MEMORY_MAXIMUM   1000

// Collect したあともヒープがいっぱいの場合、ヒープサイズを SIZE = SIZE * N の計算で増やします。
// これは、その式における N の数です。
#define HEAP_SIZE_EXTEND_COEFFICIENT  2

#define LOCK(...)  { std::lock_guard __lock(mtx); { __VA_ARGS__ } }

namespace metro::GC {

using namespace objects;

namespace {

size_t Heap_MaximumSize = OBJECT_MEMORY_MAXIMUM;

bool _isEnabled;
bool _isBusy;
bool _isKilled;
std::mutex mtx;
std::thread* thread;
size_t mark_count;

static std::vector<Object*> _Memory;
static std::vector<Object*> _Root;

Object* mAppend(Object* object) {
  std::lock_guard Lock(mtx);

  for( auto&& p : _Memory ) {
    if( p == nullptr )
      return p = object;
  }

  return _Memory.emplace_back(object);
}

void _ExtendHeap() {
  Heap_MaximumSize *= HEAP_SIZE_EXTEND_COEFFICIENT;
}

void _Bind(Object* object) {
  object->refCount++;

  if( std::find(_Root.cbegin(), _Root.cend(), object) == _Root.cend() ) {
    LOCK(
      _Root.emplace_back(object);
    )
  }
}

void _Unbind(Object* object) {
  if( --object->refCount != 0 )
    return;

  if( auto it = std::find(_Root.cbegin(), _Root.cend(), object); it != _Root.cend() ) {
    LOCK(
      _Root.erase(it);
    )
  }
}

void _Mark(Object* object) {
  object->isMarked = true;
  mark_count++;

  switch( object->type.kind ) {
    case Type::String: {
      for( auto&& c : object->as<String>()->value )
        _Mark(c);

      break;
    }

    case Type::Vector:
    case Type::Struct:
    {
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

    case Type::Tuple: {
      auto tu = object->as<Tuple>();

      for( auto&& e : tu->elements )
        _Mark(e);

      break;
    }

    case Type::Pair: {
      auto x = object->as<Pair>();

      _Mark(x->first);
      _Mark(x->second);

      break;
    }
  }
}

void _MarkAll() {
  mark_count = 0;

  for( auto&& r : _Root )
    _Mark(r);
}

void _CollectThreadFunc() {
  alertmsg("GC: start");

  _MarkAll();

  // 収集できる余地がない場合、ヒープサイズを増やして中断する。
  if( mark_count >= Heap_MaximumSize ) {
    _ExtendHeap();

    LOCK(
      for( auto&& obj : _Memory )
        obj->isMarked = false;
    )

    thread->detach();
    delete thread;

    thread = nullptr;
    
    _isBusy = false;

    alertmsg("GC: end (heap extended to " << Heap_MaximumSize << ")");
    return;
  }

  std::vector<Object*> NewHeap;

  LOCK(
    NewHeap = std::move(_Memory);
  )

  for( auto it = NewHeap.begin(); it != NewHeap.end(); ) {
    if( *it && (*it)->refCount == 0 && !(*it)->noDelete && !(*it)->isMarked ) {
      delete *it;

      NewHeap.erase(it);

      continue;
    }
    else {
      (*it++)->isMarked = false;
    }
  }

  LOCK(
    for( auto&& obj : _Memory ) {
      NewHeap.emplace_back(obj);
    }

    _Memory = std::move(NewHeap);
  )

  debug(LOCK(
    for(auto&&x:_Memory)assert(x!=nullptr);
  ))

  _isBusy = false;

  alertmsg("GC: end");
}

void _Collect() {

  if( _isBusy )
    return;

  if( thread ) {
    thread->join();
    delete thread;
  }

  _isBusy = true;
  _isKilled = false;

  thread = new std::thread(_CollectThreadFunc);
}

} // unonymous

void initialize() {
  _isEnabled = true;

  _Memory.reserve(Heap_MaximumSize);
}

bool isEnabled() {
  return _isEnabled;
}

Object* _registerObject(Object* object) {
  if( !isEnabled() )
    return object;

  if( _Memory.size() >= Heap_MaximumSize ) {
    _Collect();
  }

  return mAppend(object);
}

void bind(Object* obj) {
  if( !obj )
    return;

  if( !isEnabled() )
    return;

  _Bind(obj);
}

void unbind(Object* obj) {
  if( !obj )
    return;

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
  _Root.clear();
}

} // namespace metro::GC