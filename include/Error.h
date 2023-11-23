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
    AST::Base* as_ast() const { return (AST::Base*)this->loc; }

    ErrorLocation(size_t pos)       : loc((void*)pos) { }
    ErrorLocation(Token* token)     : loc((void*)token) { }
    ErrorLocation(AST::Base* ast)   : loc((void*)ast) { }

  private:
    ErrorLocation() { }

    void* loc;
  };

public:
  Error(ErrorLocation location);

  Error& setMessage(std::string const& msg);
  Error& emit();

  [[noreturn]]
  void exit();


private:

  ErrorLocation location;
  std::string message;

};

} // namespace metro