/*----------------------------

  pseudo mark and sweep garbage collector.

  2023/11/22  (c) aoki

  ----------------------------*/

/* todo: translate all comments to English */

#include <map>
#include <thread>
#include <mutex>
#include <memory>
#include "gc.h"
#include "alert.h"

//  the maximum count of object in memory.
//  注意：厳密には、この数に到達する手前になった段階で GC が作動します。
#define OBJECT_MEMORY_MAXIMUM   40

#define LOCK(...) std::lock_guard __lock(mtx); { __VA_ARGS__ }

namespace metro::gc {

using namespace objects;

namespace {

bool isPaused;
bool isBusy;
std::mutex mtx;
std::thread* thread;
size_t mark_count;

std::vector<Base*> _Memory;
std::vector<Base*> root; /* binded objects for local-v */

std::vector<Base*>::iterator mFind(Base* object) {
  return std::find(_Memory.begin(), _Memory.end(), object);
}

void mAppend(Base* object) {
  for( auto&& p : _Memory )
    if( p == nullptr ) {
      p = object;
      return;
    }

  _Memory.emplace_back(object);
}

/* only delete a pointer from vector. don't call free or delete. */
void mDelete(Base* object) {
  *mFind(object) = nullptr;
}

void _Bind(Base* object) {
  if( std::find(root.cbegin(), root.cend(), object) == root.cend() ) {
    LOCK(
      root.emplace_back(object);
    )
  }
}

void _Unbind(Base* object) {
  if( auto it = std::find(root.cbegin(), root.cend(), object); it != root.cend() ) {
    LOCK(
      root.erase(it);
    )
  }
}

void _Mark(Base* object) {
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

ObjectBinder::ObjectBinder(Base* obj)
  : object(obj)
{
  _Bind(obj);
}

ObjectBinder::~ObjectBinder()
{
  _Unbind(this->object);
}

void ObjectBinder::reset(Base* obj) {
  _Unbind(this->object);
  _Bind(this->object = obj);
}

Base* ObjectBinder::get() const {
  return this->object;
}

void pause() {
  isPaused = true;
}

void resume() {
  isPaused = false;
}

void addObject(objects::Base* object) {
  if( _Memory.size() >= OBJECT_MEMORY_MAXIMUM ) {
    _Collect();
  }

  mAppend(object);
}

ObjectBinder bind(objects::Base* object) {
  return ObjectBinder(object);
}

void do_collect_force() {
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

} // namespace metro::gc