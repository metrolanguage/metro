#include <algorithm>
#include <fstream>
#include "Utils.h"
#include "metro.h"

namespace metro::utils {

std::string open_text_file(std::string const& path) {
  std::ifstream ifs{ path };

  if( ifs.fail() ) {
    Metro::getInstance()->fatalError("cannot open file '" + path + "'");
  }

  std::string ret, line;

  while( std::getline(ifs, line) )
    ret += line + '\n';

  return ret;
}

} // namespace metro::utils