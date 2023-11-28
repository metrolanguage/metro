#include <iostream>
#include "alert.h"
#include "Metro.h"
#include "Error.h"

namespace metro {

std::vector<Error> Metro::emittedErrors;

Error::Error(ErrorLocation location)
  : location(location)
{
}

Error& Error::setMessage(std::string const& msg) {
  this->message = msg;

  return *this;
}

Error& Error::emit() {
  size_t beginPos = 0;
  size_t endPos   = 0;
  SourceLoc const* source = nullptr;

  switch( this->location.kind ) {
    case ErrorLocation::LC_Token: {
      alert;
      auto tok = (Token*)this->location.loc;

      beginPos = tok->position;
      endPos = tok->getEndPos();
      source = tok->source;

      break;
    }

    case ErrorLocation::LC_AST: {
      alert;

      auto ast = (AST::Base*)this->location.loc;

      beginPos = ast->token->position;
      endPos = ast->token->getEndPos();
      source = ast->token->source;

      break;
    }
  }

  size_t trimBegin  = 0;
  size_t trimEnd    = source->data.length();

  size_t linenum = 1;

  for( size_t i = 0; i < beginPos; i++ ) {
    if( source->data[i] == '\n' ) {
      linenum++;
      trimBegin = i + 1;
    }
  }

  for( trimEnd = endPos; trimEnd < source->data.length(); trimEnd++ ) {
    if( source->data[trimEnd] == '\n' )
      break;
  }

  auto errline =
    source->data.substr(trimBegin, trimEnd - trimBegin);

  std::cout
    << COL_BOLD COL_RED << "error: " << this->message << COL_DEFAULT << std::endl
    << "   --> " << source->path << ":" << linenum << std::endl
    << "    |" << std::endl
    << utils::format("% 3zu | ", linenum) << errline << std::endl
    << "    | " << std::string(beginPos - trimBegin, ' ') << "^" << std::endl
    << "    |" << std::endl;

  Metro::emittedErrors.emplace_back(*this);

  return *this;
}

void Error::exit() {
  std::exit(1);
}

void Error::check() {
  if( !Metro::emittedErrors.empty() )
    std::exit(1);
}

} // namespace metro