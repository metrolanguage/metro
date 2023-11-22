#include "lex.h"
#include "SourceLoc.h"

namespace metro {

Lexer::Lexer(SourceLoc const& src)
  : loc(src),
    source(
{
}

Lexer::~Lexer() {

}

Token* Lexer::lex() {

}

bool Lexer::check() {

}

char Lexer::peek() {

}

bool Lexer::match(std::string_view s) {

}

size_t Lexer::pass_space() {

}

size_t Lexer::pass_while(std::functional<bool(char)> cond) {

}

} // namespace metro