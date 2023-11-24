#include <iostream>

#include "metro.h"
#include "color.h"
#include "SourceLoc.h"
#include "Error.h"

#include "Lexer.h"
#include "Parser.h"
#include "Checker.h"
#include "Evaluator.h"
#include "GC.h"

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

  GC::enable();

  SourceLoc source{ "test.metro" };

  Lexer lexer{ source };

  auto token = lexer.lex();

  Error::check();

  Parser parser{ token };

  auto ast = parser.parse();

  Error::check();

  Checker checker{ ast->as<AST::Scope>() };

  checker.check(ast);

  Error::check();

  Evaluator eval;

  eval.eval(ast);

  GC::doCollectForce();
  GC::exitGC();

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