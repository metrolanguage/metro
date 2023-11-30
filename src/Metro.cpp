#include <iostream>
#include <cstdlib>
#include <filesystem>

#include "alert.h"

#include "Metro.h"
#include "Color.h"
#include "SourceLoc.h"
#include "Error.h"

#include "Lexer.h"
#include "Parser.h"
#include "Evaluator.h"
#include "GC.h"

static constexpr auto helpMessage = R"xyz(
Metro v0.0.1
(C) Aoki & all members of the team metrolanguage

usage: metro [options] [script files...]

options:
  -h --help     show this info
)xyz";

namespace metro {

static Metro* g_instance;

static std::string application_cwd;

Metro::ScriptInfo::ScriptInfo(std::string const& path)
  : source(path),
    cwd(application_cwd),
    token(nullptr),
    ast(nullptr),
    result(nullptr)
{
  for( int i = path.size(); i >= 0; i-- ) {
    if( path[i] == '/' ) {
      this->cwd += path.substr(0, i + 1);
      break;
    }
  }

  this->source.open();
}

Metro::ScriptInfo::~ScriptInfo()
{
  delete this->result;
  delete this->ast;
  delete this->token;

  for( auto&& S : this->_imported )
    delete S;
}

Metro::ScriptInfo* Metro::ScriptInfo::import(std::string const& path) {
  alertmsg("import: " << this->cwd);
  alertmsg("import: " << path);

  return this->_imported.emplace_back(new ScriptInfo(this->cwd + path));
}

Metro::Metro(int argc, char** argv)
  : currentScript(nullptr)
{
  g_instance = this;
  application_cwd = std::filesystem::current_path().string() + "/";
  
  while( argc-- )
    this->args.emplace_back(*argv++);
}

Metro::~Metro()
{
}

int Metro::main() {

  srand((unsigned)time(nullptr));

  GC::initialize();

  this->parseArguments();

  if( this->scripts.empty() ) {
    std::cout << "metro: no input files." << std::endl;
    std::exit(0);
  }

  for( auto&& script : this->scripts ) {
    this->evaluateScript(script);
  }

  GC::exitGC();

  return 0;
}

Metro::ScriptInfo* Metro::getRunningScript() {
  return this->currentScript;
}

void Metro::fatalError(std::string const& msg) {
  std::cout << Color::Red << "fatal error: " << Color::Default << msg << std::endl;
  std::exit(1);
}

Metro* Metro::getInstance() {
  return g_instance;
}

void Metro::evaluateScript(Metro::ScriptInfo& script) {
  this->currentScript = &script;

  script.source.open();

  Lexer lexer{ script.source };

  script.token = lexer.lex();

  Error::check();

  Parser parser{ script.token };

  script.ast = parser.parse();

  Error::check();

  Evaluator eval{ script.ast->as<AST::Scope>() };

  eval.eval(script.ast);

  GC::doCollectForce();
}

void Metro::parseArguments() {

  for( auto it = args.begin() + 1; it != args.end(); it++ ) {
    auto& arg = *it;

    if( arg == "-h" || arg == "--help" ) {
      std::cout << helpMessage << std::endl;
      std::exit(1);
    }
    else {
      this->scripts.emplace_back(arg);
    }
  }
}

} // namespace metro