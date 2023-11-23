#include "lex.h"
#include "SourceLoc.h"

namespace metro {

static char const* punctuaters[] {
  "+",
  "-",
  "*",
  "/",
};

Lexer::Lexer(SourceLoc const& src)
  : loc(src),
    source(src.data),
    position(0)
{
}

Lexer::~Lexer()
{
}

Token* Lexer::lex() {
  Token top;
  Token* cur = &top;

  this->pass_space();

  while( this->check() ) {
    auto str = this->source.data() + this->position;
    auto pos = this->position;
    auto ch = this->peek();

    if( isdigit(ch) ) {
      cur = new Token(TokenKind::Int, cur, { str, this->pass_while(isdigit) }, pos);
    }
    else {
      for( std::string_view pu : punctuaters ) {
        if( this->match(pu) ) {
          cur = new Token(TokenKind::Punctuater, cur, pu, pos);
          this->position += pu.length();
          goto found_punctuater;
        }
      }

      

    found_punctuater:;
    }

    this->pass_space();
  }

  cur = new Token(TokenKind::End, cur, "", this->position);

  auto ret = top.next;

  top.next = nullptr;

  return ret;
}

bool Lexer::check() {
  return this->position < this->source.length();
}

char Lexer::peek() {
  return this->source[this->position];
}

bool Lexer::match(std::string_view s) {
  if( this->position + s.length() <= this->source.length() )
    return this->source.substr(this->position, s.length()) == s;

  return false;
}

size_t Lexer::pass_space() {
  return this->pass_while(isspace);
}

size_t Lexer::pass_while(std::function<bool(char)> cond) {
  size_t count = 0;

  while( cond(this->peek()) )
    count++, this->position++;

  return count;
}

std::string_view Lexer::eat_literal(char quat) {
  auto pos = this->position++;

  while( this->peek() != quat )
    this->position++;

  return { this->source.data() + pos, (++this->position) - pos };
}

} // namespace metro