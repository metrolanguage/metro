#include <iostream>
#include "metro.h"
#include "gc.h"
#include "color.h"
#include "SourceLoc.h"
#include "lex.h"
#include "parse.h"
#include "check.h"
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

int Metro::main() {

  SourceLoc source{ "test.metro" };

  Lexer lexer{ source };

  auto token = lexer.lex();

  Parser parser{ token };

  auto ast = parser.parse();

  Checker checker{ ast->as<AST::Scope>() };

  checker.check(ast);

  Evaluator eval;

  (void)ast;

  // std::cout << eval.eval(ast)->to_string() << std::endl;


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