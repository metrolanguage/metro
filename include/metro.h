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

/*
 * Metro driver.
 */

class Error;
class Metro {
public:
  struct ScriptInfo {
    SourceLoc  source;
    
    Token*            token;
    AST::Base*        ast;
    objects::Object*  result;

    ScriptInfo(std::string const& path);
    ~ScriptInfo();
  };

  Metro(int argc, char** argv);
  ~Metro();

  int main();

  ScriptInfo const* getRunningScript();

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

  ScriptInfo const* currentScript;

  static std::vector<Error> emittedErrors; /* in Error.cpp */

  friend class Error;
};

} // namespace metro