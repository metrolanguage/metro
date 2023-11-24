#include "GC.h"

namespace metro::objects {

None None::_none;

Object::Object(Type type)
  : type(std::move(type)),
    isMarked(false)
{
  GC::_registerObject(this);
}

} // namespace metro::objects