#pragma once

#include <string>
#include <vector>

namespace metro {

// Metro driver.

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

};

} // namespace metro