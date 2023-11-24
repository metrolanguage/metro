// color for console

#pragma once

#include <string>
#include <cstdint>
#include "Utils.h"

namespace metro {

class Color {
public:
  union {
    uint32_t val;

    struct {
      uint8_t r, g, b, a;
    };
  };

  bool isBackground;

  Color(uint32_t col = 0)
    : val(col)
  {
  }

  Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
    : r(r),
      g(g),
      b(b),
      a(a)
  {
  }

  operator std::string() const {
    return utils::format("\033[38;2;%d;%d;%dm", r, g, b);
  }

  Color asBackground() {
    Color col { *this };
    
    col.isBackground = true;

    return col;
  }

  static const Color
    Default,
    Black,
    Red,
    Green,
    Yellow,
    Blue,
    Magenta,
    Cyan,
    White;
};

std::ostream& operator << (std::ostream&, Color const&);

} // namespace metroS