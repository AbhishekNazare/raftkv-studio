#pragma once

#include <optional>
#include <vector>

#include "raftkv/common/result.h"
#include "raftkv/common/status.h"
#include "raftkv/common/types.h"
#include "raftkv/raft/log_entry.h"
#include "raftkv/raft/snapshot.h"

namespace raftkv::raft {

class RaftLog {
 public:
  [[nodiscard]] bool empty() const;
  [[nodiscard]] std::size_t size() const;
  [[nodiscard]] LogIndex last_index() const;
  [[nodiscard]] Term last_term() const;
  [[nodiscard]] LogIndex commit_index() const;
  [[nodiscard]] SnapshotMetadata snapshot_metadata() const;
  [[nodiscard]] const std::vector<LogEntry>& entries() const;

  Result<LogEntry> entry_at(LogIndex index) const;
  Term term_at(LogIndex index) const;
  bool matches_at(LogIndex index, Term term) const;

  LogEntry append(Term term, std::string command);
  Status append_from_leader(LogIndex prev_log_index,
                            Term prev_log_term,
                            const std::vector<LogEntry>& leader_entries);
  Status advance_commit_index(LogIndex new_commit_index);
  Status compact_up_to(LogIndex index);
  Status install_snapshot_metadata(SnapshotMetadata metadata);

 private:
  [[nodiscard]] std::optional<std::size_t> offset_for(LogIndex index) const;
  void truncate_from(LogIndex index);

  std::vector<LogEntry> entries_;
  LogIndex commit_index_{0};
  SnapshotMetadata snapshot_metadata_;
};

}  // namespace raftkv::raft
