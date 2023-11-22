#include <iostream>
#include <string>
#include <vector>
#include "alert.h"
#include "gc.h"

using stringVector = std::vector<std::string>;

using namespace metro;

int main(int, char**) {

  using namespace objects;

  auto obj = gc::newObject<Int>(10);

  std::cout << obj->value << std::endl;

  auto vec = gc::newObject<Vector>();

  {
    auto binder = gc::bind(vec);

    for(int i=0;i<100;i++)
      gc::newObject<Int>(i);

    std::cout << vec->to_string() << std::endl;
  }
  
  gc::do_collect_force();
  gc::clean();
}