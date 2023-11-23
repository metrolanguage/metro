#include <iostream>
#include "metro.h"
#include "gc.h"
#include "color.h"
#include "SourceLoc.h"
#include "lex.h"
#include "parse.h"
#include "eval.h"

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

static void test() {
  using namespace objects;

  std::cout << Color::Red << "hello, World!\n" << Color::Default;

  auto obj = gc::newObject<Int>(10);

  std::cout << obj->value << std::endl;

  auto vec = gc::newObject<Vector>();

  {
    auto binder = gc::make_binder(vec);

    for(int i=0;i<100;i++)
      gc::newObject<Int>(i);

    std::cout << vec->to_string() << std::endl;
  }

}

int Metro::main() {

  SourceLoc source{ "test.metro" };

  Lexer lexer{ source };

  auto token = lexer.lex();

  Parser parser{ token };

  auto ast = parser.parse();

  Evaluator eval;

  std::cout << eval.eval(ast)->to_string() << std::endl;


  gc::doCollectForce();
  gc::clean();

  return 0;
}

void Metro::fatalError(std::string const& msg) {
  std::cout << Color::Red << "fatal error: " << Color::Default << msg << std::endl;
  std::exit(1);
}

Metro* Metro::getInstance() {
  return g_instance;
}


} // namespace metro