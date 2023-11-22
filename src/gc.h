#pragma once

#include "object.h"

namespace metro::gc {

struct ObjectBinder {
  void reset(objects::Base*);
  objects::Base* get() const;

  ObjectBinder(objects::Base*);
  ~ObjectBinder();

private:
  objects::Base* object;
};

void pause();
void resume();

void addObject(objects::Base*);
ObjectBinder bind(objects::Base*);

void do_collect_force();
void clean();

template <std::derived_from<objects::Base> T, class... Ts>
T* newObject(Ts&&... args) {
  auto obj = new T(std::forward<Ts>(args)...);

  addObject(obj);

  return obj;
}

} // namespace metro::gc