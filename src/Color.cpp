#include "Color.h"

namespace metro {

Color const Color::Default   { 200, 200, 200 };
Color const Color::Black     { 0, 0, 0 };
Color const Color::Red       { 255, 0, 0 };
Color const Color::Green     { 0, 255, 0 };
Color const Color::Yellow    { 255, 255, 0 };
Color const Color::Blue      { 0, 0, 255 };
Color const Color::Magenta   { 255, 0, 255 };
Color const Color::Cyan      { 87, 154, 205 };
Color const Color::White     { 255, 255, 255 };

std::ostream& operator << (std::ostream& ost, Color const& color) {
  return ost << (std::string)color;
}

} // namespace metro
