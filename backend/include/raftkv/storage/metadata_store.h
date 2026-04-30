#pragma once

#include <optional>

#include "raftkv/common/result.h"
#include "raftkv/common/status.h"
#include "raftkv/common/types.h"

namespace raftkv::storage {

struct PersistentMetadata {
  Term current_term{0};
  std::optional<NodeId> voted_for;
};

class MetadataStore {
 public:
  virtual ~MetadataStore() = default;

  virtual Status save(const PersistentMetadata& metadata) = 0;
  virtual Result<PersistentMetadata> load() const = 0;
};

}  // namespace raftkv::storage

