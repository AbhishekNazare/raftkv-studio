#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "raftkv/net/raft_rpc_mapper.h"

namespace {

void expect(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    std::exit(1);
  }
}

void test_request_vote_round_trip() {
  const raftkv::raft::RequestVoteRequest request{4, "node2", 12, 3};
  const raftkv::raft::RequestVoteRequest converted =
      raftkv::net::from_proto(raftkv::net::to_proto(request));

  expect(converted.term == 4, "term should round-trip");
  expect(converted.candidate_id == "node2", "candidate id should round-trip");
  expect(converted.last_log_index == 12, "last log index should round-trip");
  expect(converted.last_log_term == 3, "last log term should round-trip");

  const raftkv::raft::RequestVoteResponse response{4, true};
  const raftkv::raft::RequestVoteResponse converted_response =
      raftkv::net::from_proto(raftkv::net::to_proto(response));

  expect(converted_response.term == 4, "response term should round-trip");
  expect(converted_response.vote_granted,
         "vote granted should round-trip");
}

void test_append_entries_round_trip() {
  const raftkv::raft::AppendEntriesRequest request{
      8,
      "node1",
      41,
      7,
      std::vector<raftkv::raft::LogEntry>{
          {42, 8, "PUT\tuser:1\tAbhishek"},
          {43, 8, "DELETE\tuser:2"},
      },
      42,
  };

  const raftkv::raft::AppendEntriesRequest converted =
      raftkv::net::from_proto(raftkv::net::to_proto(request));

  expect(converted.term == 8, "append term should round-trip");
  expect(converted.leader_id == "node1", "leader id should round-trip");
  expect(converted.prev_log_index == 41,
         "previous log index should round-trip");
  expect(converted.prev_log_term == 7, "previous log term should round-trip");
  expect(converted.leader_commit == 42, "leader commit should round-trip");
  expect(converted.entries.size() == 2, "entries should round-trip");
  expect(converted.entries[0].index == 42, "first entry index should round-trip");
  expect(converted.entries[0].term == 8, "first entry term should round-trip");
  expect(converted.entries[0].command == "PUT\tuser:1\tAbhishek",
         "first entry command should round-trip");
  expect(converted.entries[1].command == "DELETE\tuser:2",
         "second entry command should round-trip");

  const raftkv::raft::AppendEntriesResponse response{8, true};
  const raftkv::raft::AppendEntriesResponse converted_response =
      raftkv::net::from_proto(raftkv::net::to_proto(response));

  expect(converted_response.term == 8, "response term should round-trip");
  expect(converted_response.success, "success should round-trip");
}

}  // namespace

int main() {
  test_request_vote_round_trip();
  test_append_entries_round_trip();

  std::cout << "PASS: rpc mapper tests\n";
  return 0;
}

