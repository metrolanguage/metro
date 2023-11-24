#pragma once

#include "Object.h"

namespace metro::GC {

using namespace objects;

void enable();
bool isEnabled();

void bind(Object*);
void unbind(Object*);

void doCollectForce();
void exitGC();

Object* _registerObject(Object* obj);

template <std::derived_from<Object> T, class... Ts>
T* newBinded(Ts&&... args) {
  return static_cast<T*>(_registerObject(new T(std::forward<Ts>(args)...)));
}

} // namespace metro::GC