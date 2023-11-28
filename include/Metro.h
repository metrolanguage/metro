#pragma once

#include <string>
#include <vector>
#include "SourceLoc.h"

namespace metro {

struct Token;

namespace AST {
  struct Base;
}

namespace objects {
  struct Object;
}

class Lexer;
class Parser;
class Error;

/*
 * Metro driver.
 */

class Metro {
public:
  struct ScriptInfo {
    SourceLoc  source;
    
    Token*            token;
    AST::Base*        ast;
    objects::Object*  result;
    std::vector<ScriptInfo*> _imported;

    ScriptInfo(std::string const& path);
    ~ScriptInfo();
  };

  Metro(int argc, char** argv);
  ~Metro();

  int main();

  ScriptInfo* getRunningScript();

  //
  // emit fatal error and exit with code 1.
  [[noreturn]]
  void fatalError(std::string const& msg);

  static Metro* getInstance();

private:

  void evaluateScript(ScriptInfo& script);

  void parseArguments();

  std::vector<std::string> args;
  std::vector<ScriptInfo> scripts;

  ScriptInfo* currentScript;

  static std::vector<Error> emittedErrors; /* in Error.cpp */

  friend class Error;
  friend class Parser;
};

} // namespace metro