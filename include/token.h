#pragma once

#include <string>

namespace metro {

enum class TokenKind {
  Unknown,

  /* numeric types */
  Int,
  Float,
  USize,

  /* boolean */
  Bool,

  /* literal */
  Char,
  String,

  Identifier,

  Punctuater,

  End
};

struct SourceLoc;
struct Token {
  TokenKind kind;
  Token* prev;
  Token* next;
  std::string_view str;
  size_t position;
  SourceLoc* source;

  size_t getEndPos() const;

  Token(TokenKind kind = TokenKind::Unknown);
  Token(TokenKind kind, Token* prev, std::string_view str, size_t pos);
  ~Token();
};

} // namespace metro