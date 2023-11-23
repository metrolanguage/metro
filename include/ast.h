#pragma once

#include <vector>
#include "token.h"
#include "object.h"

namespace metro {
  
enum class ASTKind {
  Value,
  Variable,

  Add,
  Sub,
};

namespace AST {

struct Base {
  ASTKind kind;
  Token* token;

  template <class T>
  T* as() { return (T*)this; }

protected:
  Base(ASTKind kind, Token* token = nullptr)
    : kind(kind),
      token(token)
  {
  }
};

struct Value : Base {
  objects::Object* object;

  Value(Token* token, objects::Object* object)
    : Base(ASTKind::Value, token),
      object(object)
  {
  }
};

struct Variable : Base {
  std::string_view getName() const {
    return this->token->str;
  }

  Variable(Token* token)
    : Base(ASTKind::Variable, token)
  {
  }
};

struct Expr : Base {
  Base* left;
  Base* right;
  
  Expr(ASTKind kind, Token* op_token, Base* left, Base* right)
    : Base(kind, op_token),
      left(left),
      right(right)
  {
  }
};

} // namespace AST

} // namespace metro