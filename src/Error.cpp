#include <iostream>
#include "Error.h"
#include "metro.h"
#include "alert.h"

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
  auto script = Metro::getInstance()->getRunningScript();
  auto const& source = script->source;

  size_t beginPos = 0;
  size_t endPos   = 0;

  size_t trimBegin  = 0;
  size_t trimEnd    = source.data.length();

  size_t linenum = 1;

  switch( this->location.kind ) {
    case ErrorLocation::LC_Position: {
      alert;
      size_t pos = (size_t)this->location.loc;

      beginPos = endPos = pos;

      break;
    }

    case ErrorLocation::LC_Token: {
      alert;
      auto tok = (Token*)this->location.loc;

      beginPos = tok->position;
      endPos = tok->getEndPos();

      break;
    }

    case ErrorLocation::LC_AST: {
      alert;

      auto ast = (AST::Base*)this->location.loc;

      beginPos = ast->token->position;
      endPos = ast->token->getEndPos();

      break;
    }
  }

  for( size_t i = 0; i < beginPos; i++ ) {
    if( source.data[i] == '\n' ) {
      linenum++;
      trimBegin = i + 1;
    }
  }

  for( trimEnd = endPos; trimEnd < source.data.length(); trimEnd++ ) {
    if( source.data[trimEnd] == '\n' )
      break;
  }

  auto errline =
    source.data.substr(trimBegin, trimEnd - trimBegin);

  std::cout
    << COL_BOLD COL_RED << "error: " << this->message << COL_DEFAULT << std::endl
    << "   --> " << source.path << ":" << linenum << std::endl
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