#include "token.h"

namespace metro {

size_t Token::getEndPos() const {
  return this->position + this->str.length();
}

Token::Token(TokenKind kind)
  : kind(kind),
    prev(nullptr),
    next(nullptr),
    str(""),
    position(0),
    source(nullptr)
{
}

Token::Token(TokenKind kind, Token* prev, std::string_view str, size_t pos)
  : kind(kind),
    prev(prev),
    next(nullptr),
    str(str),
    position(pos),
    source(nullptr)
{
  if( prev )
    prev->next = this;
}

Token::~Token()
{
  if( this->next )
    delete this->next;
}

} // namespace metro