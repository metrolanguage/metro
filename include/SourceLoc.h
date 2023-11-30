#pragma once

#include <string>

namespace metro {

struct SourceLoc {
  std::string path;
  std::string data;

  void open();

  SourceLoc();
  SourceLoc(std::string const& path);
};

} // namespace metro