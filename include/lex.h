#pragma once

#include <functional>
#include "token.h"

namespace metro {

class Lexer {
public:
  Lexer(SourceLoc const&);
  ~Lexer();

  Token* lex();

private:
  bool check();
  char peek();
  bool match(std::string_view s);
  size_t pass_space();
  size_t pass_while(std::functional<bool(char)> cond);

  SourceLoc const& loc;
  std::string const& source;
  size_t position;
};

} // namespace metro