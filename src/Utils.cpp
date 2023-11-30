#include <algorithm>
#include <fstream>
#include <filesystem>
#include "Utils.h"
#include "Metro.h"

namespace metro::utils {

std::string join(std::string str, std::vector<std::string> const& vec) {
  if( vec.empty() )
    return "";

  std::string ret;

  for( auto&& s : vec ) {
    ret += s;

    if( &s != &*vec.rbegin() )
      ret += str;
  }

  return ret;
}

std::vector<std::string> split(std::string str, char c) {
  std::vector<std::string> ret;

  for( size_t pos = 0; (pos = str.find(c)) != std::string::npos; ) {
    if( pos != 0 )
      ret.emplace_back(str.substr(0, pos));

    str = str.substr(pos + 1);
  }

  if( !str.empty() )
    ret.emplace_back(str);

  return ret;
}

//
// Open a text file
//
std::string open_text_file(std::string const& path) {
  std::ifstream ifs{ path };

  // don't open directory.
  if( std::filesystem::is_directory(path) )
    Metro::getInstance()->fatalError("'" + path + "' is directory");

  if( ifs.fail() ) {
    Metro::getInstance()->fatalError("cannot open file '" + path + "'");
  }

  std::string ret, line;

  while( std::getline(ifs, line) )
    ret += line + '\n';

  return ret;
}

} // namespace metro::utils