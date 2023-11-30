#include "alert.h"
#include "SourceLoc.h"
#include "Utils.h"

namespace metro {

SourceLoc::SourceLoc()
{
}

SourceLoc::SourceLoc(std::string const& path)
  : path(path)
{
}

void SourceLoc::open() {
  this->data = utils::open_text_file(this->path);
}

} // namespace metro