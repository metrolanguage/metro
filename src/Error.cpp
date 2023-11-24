#include <iostream>
#include "Error.h"
#include "metro.h"

namespace metro {

std::vector<Error> Metro::emittedErrors;

Error::Error(ErrorLocation location)
  : location(location)
{
}

Error& Error::setMessage(std::string const& msg) {
  this->message = msg;

  return *this;
}

Error& Error::emit() {
  std::cout << "error: " << this->message << std::endl;

  Metro::emittedErrors.emplace_back(*this);

  return *this;
}

void Error::exit() {
  std::exit(1);
}

void Error::check() {
  if( !Metro::emittedErrors.empty() )
    std::exit(1);
}

} // namespace metro