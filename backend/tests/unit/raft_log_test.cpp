#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "raftkv/raft/raft_log.h"

namespace {

void expect(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    std::exit(1);
  }
}

void test_append_assigns_monotonic_indexes() {
  raftkv::raft::RaftLog log;

  const raftkv::raft::LogEntry first = log.append(1, "PUT user:1 Abhishek");
  const raftkv::raft::LogEntry second = log.append(1, "PUT user:2 Maya");

  expect(first.index == 1, "first appended entry should be index 1");
  expect(second.index == 2, "second appended entry should be index 2");
  expect(log.last_index() == 2, "last index should track latest entry");
  expect(log.last_term() == 1, "last term should track latest entry term");
  expect(log.size() == 2, "log should contain two entries");
}

void test_lookup_and_matching() {
  raftkv::raft::RaftLog log;
  log.append(1, "PUT a 1");
  log.append(2, "PUT b 2");

  const raftkv::Result<raftkv::raft::LogEntry> entry = log.entry_at(2);
  expect(entry.ok(), "entry at index 2 should exist");
  expect(entry.value().term == 2, "entry term should match");
  expect(entry.value().command == "PUT b 2", "entry command should match");

  expect(log.matches_at(0, 0), "sentinel previous entry should match");
  expect(log.matches_at(1, 1), "existing index and term should match");
  expect(!log.matches_at(1, 2), "wrong term should not match");
  expect(!log.matches_at(9, 1), "missing index should not match");
}

void test_append_from_leader_requires_previous_entry_match() {
  raftkv::raft::RaftLog follower;
  follower.append(1, "PUT a 1");

  const raftkv::Status status = follower.append_from_leader(
      1, 2, std::vector<raftkv::raft::LogEntry>{{2, 2, "PUT b 2"}});

  expect(!status.ok_status(), "append should reject mismatched prev term");
  expect(status.code() == raftkv::StatusCode::kInvalidArgument,
         "mismatched prev term should be invalid argument");
  expect(follower.last_index() == 1, "rejected append should not mutate log");
}

void test_append_from_leader_adds_missing_entries() {
  raftkv::raft::RaftLog follower;
  follower.append(1, "PUT a 1");

  const raftkv::Status status = follower.append_from_leader(
      1, 1,
      std::vector<raftkv::raft::LogEntry>{
          {2, 1, "PUT b 2"},
          {3, 1, "PUT c 3"},
      });

  expect(status.ok_status(), "leader append should succeed");
  expect(follower.last_index() == 3, "follower should append missing entries");
  expect(follower.entry_at(3).value().command == "PUT c 3",
         "last appended command should match leader entry");
}

void test_conflicting_suffix_is_replaced() {
  raftkv::raft::RaftLog follower;
  follower.append(1, "PUT a 1");
  follower.append(2, "PUT stale value");
  follower.append(2, "PUT stale suffix");

  const raftkv::Status status = follower.append_from_leader(
      1, 1,
      std::vector<raftkv::raft::LogEntry>{
          {2, 3, "PUT fresh value"},
          {3, 3, "PUT fresh suffix"},
      });

  expect(status.ok_status(), "conflicting uncommitted suffix should be replaced");
  expect(follower.size() == 3, "replacement should keep leader-sized log");
  expect(follower.entry_at(2).value().term == 3,
         "conflicting entry should have new leader term");
  expect(follower.entry_at(2).value().command == "PUT fresh value",
         "conflicting command should be replaced");
  expect(follower.entry_at(3).value().command == "PUT fresh suffix",
         "suffix should match leader");
}

void test_committed_entries_are_not_replaced() {
  raftkv::raft::RaftLog follower;
  follower.append(1, "PUT a 1");
  follower.append(2, "PUT committed");
  expect(follower.advance_commit_index(2).ok_status(),
         "commit index should advance");

  const raftkv::Status status = follower.append_from_leader(
      1, 1, std::vector<raftkv::raft::LogEntry>{{2, 3, "PUT overwrite"}});

  expect(!status.ok_status(), "committed entry replacement should fail");
  expect(follower.entry_at(2).value().command == "PUT committed",
         "committed entry should remain unchanged");
}

void test_commit_index_only_moves_forward_within_log() {
  raftkv::raft::RaftLog log;
  log.append(1, "PUT a 1");
  log.append(1, "PUT b 2");

  expect(log.advance_commit_index(1).ok_status(),
         "commit index should advance to existing entry");
  expect(log.commit_index() == 1, "commit index should be updated");

  const raftkv::Status backward = log.advance_commit_index(0);
  expect(!backward.ok_status(), "commit index should not move backward");

  const raftkv::Status past_end = log.advance_commit_index(3);
  expect(!past_end.ok_status(), "commit index should not pass last index");
  expect(log.commit_index() == 1,
         "failed commit updates should leave commit index unchanged");
}

}  // namespace

int main() {
  test_append_assigns_monotonic_indexes();
  test_lookup_and_matching();
  test_append_from_leader_requires_previous_entry_match();
  test_append_from_leader_adds_missing_entries();
  test_conflicting_suffix_is_replaced();
  test_committed_entries_are_not_replaced();
  test_commit_index_only_moves_forward_within_log();

  std::cout << "PASS: raft log tests\n";
  return 0;
}

