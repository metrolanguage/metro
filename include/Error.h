#pragma once

#include <string>
#include "AST.h"

namespace metro {

class Error {
  struct ErrorLocation {
    enum Kind {
      LC_Token,
      LC_AST,
    };

    Kind kind;
    void* loc;

    Token* as_token() const { return (Token*)this->loc; }
    AST::Base* as_ast() const { return (AST::Base*)this->loc; }

    ErrorLocation(Token* token)
      : kind(LC_Token),
        loc((void*)token)
    {
    }

    ErrorLocation(AST::Base* ast)
      : kind(LC_AST),
        loc((void*)ast)
    {
    }

  private:
    ErrorLocation() { }
  };

public:
  Error(ErrorLocation location);

  Error& setMessage(std::string const& msg);
  Error& emit();

  [[noreturn]]
  void exit();

  static void check();

private:

  ErrorLocation location;
  std::string message;

};

} // namespace metro