#pragma once

#include <vector>

#include "raftkv/common/result.h"
#include "raftkv/common/status.h"
#include "raftkv/raft/log_entry.h"

namespace raftkv::storage {

class LogStore {
 public:
  virtual ~LogStore() = default;

  virtual Status append(const raft::LogEntry& entry) = 0;
  virtual Status overwrite_all(const std::vector<raft::LogEntry>& entries) = 0;
  virtual Result<std::vector<raft::LogEntry>> load_all() const = 0;
};

}  // namespace raftkv::storage

