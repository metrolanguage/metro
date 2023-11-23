#include "SourceLoc.h"
#include "utils.h"

namespace metro {

SourceLoc::SourceLoc()
{
}

SourceLoc::SourceLoc(std::string const& path)
  : path(path),
    data(utils::open_text_file(path))
{
}

} // namespace metro