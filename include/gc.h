#pragma once

#include "object.h"

namespace metro::gc {

struct ObjectBinder {
  void reset(objects::Object*);
  objects::Object* get() const;

  ObjectBinder(objects::Object*);
  ~ObjectBinder();

private:
  objects::Object* object;
};

void pause();
void resume();

void addObject(objects::Object*);
ObjectBinder make_binder(objects::Object*);

void bind(objects::Object*);
void unbind(objects::Object*);

void doCollectForce();
void clean();

objects::Object* cloneObject(objects::Object*);

template <std::derived_from<objects::Object> T, class... Ts>
T* newObject(Ts&&... args) {
  auto obj = new T(std::forward<Ts>(args)...);

  addObject(obj);

  return obj;
}

template <std::derived_from<objects::Object> T, class... Ts>
T* newBinded(Ts&&... args) {
  auto obj = newObject<T>(std::forward<Ts>(args)...);

  bind(obj);

  return obj;
}

} // namespace metro::gc