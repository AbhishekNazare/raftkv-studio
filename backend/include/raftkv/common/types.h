#pragma once

#include <cstdint>
#include <string>

namespace raftkv {

using NodeId = std::string;
using Term = std::uint64_t;
using LogIndex = std::uint64_t;

struct Version {
  int major;
  int minor;
  int patch;
};

Version backend_version();
std::string backend_version_string();

}  // namespace raftkv

