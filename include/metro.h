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
  };

  Metro(int argc, char** argv);
  ~Metro();

  int main();


  //
  // emit fatal error and exit with code 1.
  [[noreturn]]
  void fatalError(std::string const& msg);


  void evaluateScript(ScriptInfo& script);


  static Metro* getInstance();

private:

  std::vector<std::string> args;
  std::vector<ScriptInfo> scripts;

  static std::vector<Error> emittedErrors; /* in Error.cpp */

  friend class Error;
};

} // namespace metro