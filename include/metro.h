#pragma once

#include <string>
#include <vector>

namespace metro {

// Metro driver.

class Error;
class Metro {
public:

  Metro(int argc, char** argv);
  ~Metro();

  int main();


  //
  // emit fatal error and exit with code 1.
  [[noreturn]]
  void fatalError(std::string const& msg);


  static Metro* getInstance();

private:

  std::vector<std::string> args;

  static std::vector<Error> emittedErrors; /* in Error.cpp */

  friend class Error;
};

} // namespace metro