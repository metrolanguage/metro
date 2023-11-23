#pragma once

#include <string>
#include "token.h"
#include "ast.h"

namespace metro {

class Error {

  struct ErrorLocation {
    enum Kind {
      LC_Position,
      LC_Token,
      LC_AST,
    };

    size_t as_position() const { return (size_t)this->loc; }
    Token* as_token() const { return (Token*)this->loc; }
    

  private:
    void* loc;
  };

public:
  Error();





private:

};

} // namespace metro