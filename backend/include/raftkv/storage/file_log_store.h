#pragma once

#include <filesystem>

#include "raftkv/storage/log_store.h"

namespace raftkv::storage {

class FileLogStore final : public LogStore {
 public:
  explicit FileLogStore(std::filesystem::path path);

  Status append(const raft::LogEntry& entry) override;
  Status overwrite_all(const std::vector<raft::LogEntry>& entries) override;
  Result<std::vector<raft::LogEntry>> load_all() const override;

 private:
  std::filesystem::path path_;
};

}  // namespace raftkv::storage

