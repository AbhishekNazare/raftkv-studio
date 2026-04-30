#include <cstdlib>
#include <iostream>
#include <string>

#include "raftkv/kv/command_codec.h"
#include "raftkv/raft/in_process_cluster.h"

namespace {

void expect(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    std::exit(1);
  }
}

const raftkv::raft::RaftNode& require_node(
    const raftkv::raft::InProcessCluster& cluster,
    const raftkv::NodeId& id) {
  const raftkv::Result<const raftkv::raft::RaftNode*> result = cluster.node(id);
  expect(result.ok(), "node should exist");
  return *result.value();
}

void test_write_commits_after_majority_replication() {
  raftkv::raft::InProcessCluster cluster({"node1", "node2", "node3"});
  expect(cluster.elect_leader("node1").ok_status(),
         "node1 should become leader");

  const std::string command =
      raftkv::kv::encode_command(raftkv::kv::Command::put("user:1", "Abhishek"));
  const raftkv::Result<raftkv::raft::ReplicationResult> result =
      cluster.replicate_command(command);

  expect(result.ok(), "replication should return result");
  expect(result.value().committed, "write should commit with all nodes alive");
  expect(result.value().acknowledgements == 3,
         "all three nodes should acknowledge");

  for (const std::string& node_id : {"node1", "node2", "node3"}) {
    const raftkv::raft::RaftNode& node = require_node(cluster, node_id);
    expect(node.log().commit_index() == 1,
           "committed write should advance each node commit index");
    expect(node.last_applied() == 1,
           "committed write should apply on each available node");

    const raftkv::Result<std::string> value =
        node.state_machine().get("user:1");
    expect(value.ok(), "committed value should be readable");
    expect(value.value() == "Abhishek", "committed value should match command");
  }
}

void test_write_commits_with_two_of_three_nodes() {
  raftkv::raft::InProcessCluster cluster({"node1", "node2", "node3"});
  expect(cluster.elect_leader("node1").ok_status(),
         "node1 should become leader");
  expect(cluster.set_available("node3", false).ok_status(),
         "node3 should be marked unavailable");

  const std::string command =
      raftkv::kv::encode_command(raftkv::kv::Command::put("order:101", "paid"));
  const raftkv::Result<raftkv::raft::ReplicationResult> result =
      cluster.replicate_command(command);

  expect(result.ok(), "replication should return result");
  expect(result.value().committed, "write should commit with 2/3 nodes");
  expect(result.value().acknowledgements == 2,
         "leader plus one follower should form majority");

  const raftkv::raft::RaftNode& leader = require_node(cluster, "node1");
  const raftkv::raft::RaftNode& follower = require_node(cluster, "node2");
  const raftkv::raft::RaftNode& offline = require_node(cluster, "node3");

  expect(leader.state_machine().get("order:101").value() == "paid",
         "leader should apply committed write");
  expect(follower.state_machine().get("order:101").value() == "paid",
         "available follower should apply committed write");
  expect(!offline.state_machine().get("order:101").ok(),
         "offline follower should not receive write yet");
  expect(offline.log().last_index() == 0,
         "offline follower log should remain behind");
}

void test_write_does_not_commit_without_quorum() {
  raftkv::raft::InProcessCluster cluster({"node1", "node2", "node3"});
  expect(cluster.elect_leader("node1").ok_status(),
         "node1 should become leader");
  expect(cluster.set_available("node2", false).ok_status(),
         "node2 should be marked unavailable");
  expect(cluster.set_available("node3", false).ok_status(),
         "node3 should be marked unavailable");

  const std::string command = raftkv::kv::encode_command(
      raftkv::kv::Command::put("payment:55", "success"));
  const raftkv::Result<raftkv::raft::ReplicationResult> result =
      cluster.replicate_command(command);

  expect(result.ok(), "replication attempt should return result");
  expect(!result.value().committed,
         "write should not commit when only leader is alive");
  expect(result.value().acknowledgements == 1,
         "only the leader should acknowledge");

  const raftkv::raft::RaftNode& leader = require_node(cluster, "node1");
  expect(leader.log().last_index() == 1,
         "leader may append uncommitted entry locally");
  expect(leader.log().commit_index() == 0,
         "leader should not commit without majority");
  expect(leader.last_applied() == 0,
         "uncommitted entry should not be applied");
  expect(!leader.state_machine().get("payment:55").ok(),
         "uncommitted value should not be readable");
}

}  // namespace

int main() {
  test_write_commits_after_majority_replication();
  test_write_commits_with_two_of_three_nodes();
  test_write_does_not_commit_without_quorum();

  std::cout << "PASS: replication tests\n";
  return 0;
}
