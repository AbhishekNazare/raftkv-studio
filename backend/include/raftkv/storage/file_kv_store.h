#pragma once

#include <filesystem>

#include "raftkv/storage/kv_store.h"

namespace raftkv::storage {

class FileKVStore final : public KVStore {
 public:
  explicit FileKVStore(std::filesystem::path path);

  Status save_snapshot(const std::map<std::string, std::string>& values) override;
  Result<std::map<std::string, std::string>> load_snapshot() const override;

 private:
  std::filesystem::path path_;
};

}  // namespace raftkv::storage

