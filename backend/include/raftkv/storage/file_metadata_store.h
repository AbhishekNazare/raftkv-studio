#pragma once

#include <filesystem>

#include "raftkv/storage/metadata_store.h"

namespace raftkv::storage {

class FileMetadataStore final : public MetadataStore {
 public:
  explicit FileMetadataStore(std::filesystem::path path);

  Status save(const PersistentMetadata& metadata) override;
  Result<PersistentMetadata> load() const override;

 private:
  std::filesystem::path path_;
};

}  // namespace raftkv::storage

