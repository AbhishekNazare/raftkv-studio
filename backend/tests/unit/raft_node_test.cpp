#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "raftkv/kv/command_codec.h"
#include "raftkv/raft/raft_node.h"

namespace {

void expect(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    std::exit(1);
  }
}

std::size_t majority(std::size_t cluster_size) {
  return (cluster_size / 2) + 1;
}

void test_node_starts_as_follower() {
  raftkv::raft::RaftNode node("node1");

  expect(node.id() == "node1", "node id should be stored");
  expect(node.role() == raftkv::raft::RaftRole::kFollower,
         "node should start as follower");
  expect(node.current_term() == 0, "node should start in term 0");
  expect(!node.voted_for().has_value(), "node should not have voted yet");
}

void test_start_election_votes_for_self() {
  raftkv::raft::RaftNode node("node1");

  node.start_election();

  expect(node.role() == raftkv::raft::RaftRole::kCandidate,
         "election should make node candidate");
  expect(node.current_term() == 1, "election should increment term");
  expect(node.voted_for().has_value(), "candidate should vote for itself");
  expect(*node.voted_for() == "node1", "candidate vote should be self vote");
}

void test_request_vote_grants_once_per_term() {
  raftkv::raft::RaftNode follower("node1");

  const raftkv::raft::RequestVoteResponse first =
      follower.handle_request_vote({1, "node2", 0, 0});
  const raftkv::raft::RequestVoteResponse duplicate =
      follower.handle_request_vote({1, "node2", 0, 0});
  const raftkv::raft::RequestVoteResponse other =
      follower.handle_request_vote({1, "node3", 0, 0});

  expect(first.vote_granted, "first candidate should receive vote");
  expect(duplicate.vote_granted, "same candidate can receive duplicate vote");
  expect(!other.vote_granted,
         "different candidate should not receive second vote in same term");
  expect(follower.voted_for().has_value(), "follower should remember vote");
  expect(*follower.voted_for() == "node2", "follower should remember candidate");
}

void test_request_vote_rejects_stale_term() {
  raftkv::raft::RaftNode follower("node1");
  follower.start_election();
  follower.start_election();

  const raftkv::raft::RequestVoteResponse response =
      follower.handle_request_vote({1, "node2", 0, 0});

  expect(!response.vote_granted, "stale term vote request should be rejected");
  expect(response.term == 2, "response should include current term");
  expect(follower.role() == raftkv::raft::RaftRole::kCandidate,
         "stale request should not force step down");
}

void test_request_vote_rejects_stale_candidate_log() {
  raftkv::raft::RaftNode follower("node1");
  follower.mutable_log().append(2, "PUT user:1 Abhishek");

  const raftkv::raft::RequestVoteResponse response =
      follower.handle_request_vote({3, "node2", 0, 0});

  expect(!response.vote_granted,
         "candidate with stale log should not receive vote");
  expect(follower.current_term() == 3,
         "newer request term should still update follower term");
  expect(!follower.voted_for().has_value(),
         "rejected stale-log candidate should not be recorded as voted for");
}

void test_candidate_steps_down_for_newer_append_entries() {
  raftkv::raft::RaftNode candidate("node1");
  candidate.start_election();

  const raftkv::raft::AppendEntriesResponse response =
      candidate.handle_append_entries({2, "node2", 0, 0, {}, 0});

  expect(response.success, "newer heartbeat with matching log should succeed");
  expect(response.term == 2, "response should include updated term");
  expect(candidate.current_term() == 2, "candidate should update term");
  expect(candidate.role() == raftkv::raft::RaftRole::kFollower,
         "candidate should step down to follower");
  expect(candidate.leader_id().has_value(), "leader id should be recorded");
  expect(*candidate.leader_id() == "node2", "leader id should match request");
  expect(!candidate.voted_for().has_value(),
         "newer term should clear previous vote");
}

void test_append_entries_rejects_stale_leader() {
  raftkv::raft::RaftNode follower("node1");
  follower.start_election();
  follower.start_election();

  const raftkv::raft::AppendEntriesResponse response =
      follower.handle_append_entries({1, "node2", 0, 0, {}, 0});

  expect(!response.success, "stale leader heartbeat should be rejected");
  expect(response.term == 2, "response should include follower current term");
  expect(follower.role() == raftkv::raft::RaftRole::kCandidate,
         "stale heartbeat should not change role");
}

void test_append_entries_validates_previous_log_entry() {
  raftkv::raft::RaftNode follower("node1");
  follower.mutable_log().append(1, "PUT a 1");

  const raftkv::raft::AppendEntriesResponse response =
      follower.handle_append_entries({1, "node2", 1, 2, {}, 0});

  expect(!response.success, "mismatched previous log term should be rejected");
  expect(follower.leader_id().has_value(),
         "same-term leader can still be observed before log validation");
}

void test_append_entries_advances_commit_to_minimum_known_index() {
  raftkv::raft::RaftNode follower("node1");
  follower.mutable_log().append(
      1, raftkv::kv::encode_command(raftkv::kv::Command::put("a", "1")));
  follower.mutable_log().append(
      1, raftkv::kv::encode_command(raftkv::kv::Command::put("b", "2")));

  const raftkv::raft::AppendEntriesResponse response =
      follower.handle_append_entries({1, "node2", 2, 1, {}, 99});

  expect(response.success, "heartbeat should succeed with matching previous log");
  expect(follower.log().commit_index() == 2,
         "commit index should advance only to local last index");
}

void test_deterministic_cluster_elects_one_leader() {
  std::vector<raftkv::raft::RaftNode> nodes;
  nodes.emplace_back("node1");
  nodes.emplace_back("node2");
  nodes.emplace_back("node3");

  nodes[1].start_election();
  std::size_t votes = 1;
  const raftkv::raft::RequestVoteRequest request =
      nodes[1].build_request_vote();

  for (std::size_t i = 0; i < nodes.size(); ++i) {
    if (i == 1) {
      continue;
    }

    const raftkv::raft::RequestVoteResponse response =
        nodes[i].handle_request_vote(request);
    if (response.vote_granted) {
      ++votes;
    }
  }

  expect(votes >= majority(nodes.size()), "candidate should receive majority");
  nodes[1].become_leader();

  std::size_t leaders = 0;
  for (const raftkv::raft::RaftNode& node : nodes) {
    if (node.role() == raftkv::raft::RaftRole::kLeader) {
      ++leaders;
    }
  }

  expect(leaders == 1, "cluster should have exactly one leader");
  expect(nodes[1].leader_id().has_value(), "leader should record itself");
  expect(*nodes[1].leader_id() == "node2", "leader id should be node2");
}

}  // namespace

int main() {
  test_node_starts_as_follower();
  test_start_election_votes_for_self();
  test_request_vote_grants_once_per_term();
  test_request_vote_rejects_stale_term();
  test_request_vote_rejects_stale_candidate_log();
  test_candidate_steps_down_for_newer_append_entries();
  test_append_entries_rejects_stale_leader();
  test_append_entries_validates_previous_log_entry();
  test_append_entries_advances_commit_to_minimum_known_index();
  test_deterministic_cluster_elects_one_leader();

  std::cout << "PASS: raft node tests\n";
  return 0;
}
