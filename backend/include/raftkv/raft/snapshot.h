#pragma once

#include <map>
#include <string>

#include "raftkv/common/types.h"

namespace raftkv::raft {

struct SnapshotMetadata {
  LogIndex last_included_index{0};
  Term last_included_term{0};
};

struct Snapshot {
  SnapshotMetadata metadata;
  std::map<std::string, std::string> kv_state;
};

}  // namespace raftkv::raft

