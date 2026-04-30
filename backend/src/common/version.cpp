#include "raftkv/common/types.h"

#include <sstream>

namespace raftkv {

Version backend_version() {
  return Version{0, 1, 0};
}

std::string backend_version_string() {
  const Version version = backend_version();

  std::ostringstream out;
  out << version.major << "." << version.minor << "." << version.patch;
  return out.str();
}

}  // namespace raftkv

