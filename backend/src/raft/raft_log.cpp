#include "raftkv/raft/raft_log.h"

#include <algorithm>
#include <utility>

namespace raftkv::raft {

bool RaftLog::empty() const {
  return entries_.empty();
}

std::size_t RaftLog::size() const {
  return entries_.size();
}

LogIndex RaftLog::last_index() const {
  if (entries_.empty()) {
    return 0;
  }

  return entries_.back().index;
}

Term RaftLog::last_term() const {
  if (entries_.empty()) {
    return 0;
  }

  return entries_.back().term;
}

LogIndex RaftLog::commit_index() const {
  return commit_index_;
}

const std::vector<LogEntry>& RaftLog::entries() const {
  return entries_;
}

Result<LogEntry> RaftLog::entry_at(LogIndex index) const {
  const std::optional<std::size_t> offset = offset_for(index);
  if (!offset.has_value()) {
    return Status::error(StatusCode::kNotFound, "log entry not found");
  }

  return entries_[*offset];
}

Term RaftLog::term_at(LogIndex index) const {
  if (index == 0) {
    return 0;
  }

  const std::optional<std::size_t> offset = offset_for(index);
  if (!offset.has_value()) {
    return 0;
  }

  return entries_[*offset].term;
}

bool RaftLog::matches_at(LogIndex index, Term term) const {
  return term_at(index) == term;
}

LogEntry RaftLog::append(Term term, std::string command) {
  const LogEntry entry{last_index() + 1, term, std::move(command)};
  entries_.push_back(entry);
  return entry;
}

Status RaftLog::append_from_leader(
    LogIndex prev_log_index,
    Term prev_log_term,
    const std::vector<LogEntry>& leader_entries) {
  if (!matches_at(prev_log_index, prev_log_term)) {
    return Status::error(StatusCode::kInvalidArgument,
                         "previous log index and term do not match");
  }

  for (const LogEntry& leader_entry : leader_entries) {
    if (leader_entry.index == 0) {
      return Status::error(StatusCode::kInvalidArgument,
                           "leader entry index must start at 1");
    }

    if (leader_entry.index <= prev_log_index) {
      return Status::error(StatusCode::kInvalidArgument,
                           "leader entry overlaps previous log index");
    }

    const LogIndex expected_index = last_index() + 1;
    if (leader_entry.index > expected_index) {
      return Status::error(StatusCode::kInvalidArgument,
                           "leader entries must not leave an index gap");
    }

    const std::optional<std::size_t> existing = offset_for(leader_entry.index);
    if (existing.has_value()) {
      if (entries_[*existing].term == leader_entry.term) {
        continue;
      }

      if (leader_entry.index <= commit_index_) {
        return Status::error(StatusCode::kInvalidArgument,
                             "cannot replace committed log entry");
      }

      truncate_from(leader_entry.index);
    }

    entries_.push_back(leader_entry);
  }

  return Status::ok();
}

Status RaftLog::advance_commit_index(LogIndex new_commit_index) {
  if (new_commit_index < commit_index_) {
    return Status::error(StatusCode::kInvalidArgument,
                         "commit index cannot move backward");
  }

  if (new_commit_index > last_index()) {
    return Status::error(StatusCode::kInvalidArgument,
                         "commit index cannot pass last log index");
  }

  commit_index_ = new_commit_index;
  return Status::ok();
}

std::optional<std::size_t> RaftLog::offset_for(LogIndex index) const {
  if (index == 0 || entries_.empty()) {
    return std::nullopt;
  }

  const LogIndex first_index = entries_.front().index;
  if (index < first_index) {
    return std::nullopt;
  }

  const std::size_t offset = static_cast<std::size_t>(index - first_index);
  if (offset >= entries_.size()) {
    return std::nullopt;
  }

  if (entries_[offset].index != index) {
    return std::nullopt;
  }

  return offset;
}

void RaftLog::truncate_from(LogIndex index) {
  entries_.erase(
      std::remove_if(entries_.begin(), entries_.end(),
                     [index](const LogEntry& entry) {
                       return entry.index >= index;
                     }),
      entries_.end());
}

}  // namespace raftkv::raft

