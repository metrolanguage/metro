#include "metro.h"

namespace metro {

static Metro* g_instance;

Metro::Metro(int argc, char** argv)
{
  g_instance = this;
  
  while( argc-- )
    this->args.emplace_back(*argv++);
}

Metro::~Metro()
{
}

int Metro::main() {


  return 0;
}

void Metro::fatalError(std::string const& msg) {

}

Metro* Metro::getInstance() {
  return g_instance;
}


} // namespace metro