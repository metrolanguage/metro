#pragma once

#include <thread>
#include <cstring>
#include <string>

namespace metro::utils {

template <class... Ts>
std::string format(char const* fmt, Ts&&... args) {
  thread_local char buf[0x1000];
  std::sprintf(buf, fmt, std::forward<Ts>(args)...);
  return buf;
}

std::string open_text_file(std::string const& path);

} // namespace metro::utils