#pragma once

#include <map>
#include <string>

#include "raftkv/common/result.h"
#include "raftkv/common/status.h"

namespace raftkv::storage {

class KVStore {
 public:
  virtual ~KVStore() = default;

  virtual Status save_snapshot(
      const std::map<std::string, std::string>& values) = 0;
  virtual Result<std::map<std::string, std::string>> load_snapshot() const = 0;
};

}  // namespace raftkv::storage

