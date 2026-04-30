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

std::string put_command(const std::string& key, const std::string& value) {
  return raftkv::kv::encode_command(raftkv::kv::Command::put(key, value));
}

const raftkv::raft::RaftNode& require_node(
    const raftkv::raft::InProcessCluster& cluster,
    const raftkv::NodeId& id) {
  const raftkv::Result<const raftkv::raft::RaftNode*> result = cluster.node(id);
  expect(result.ok(), "node should exist");
  return *result.value();
}

void test_lagging_follower_catches_up_after_restart() {
  raftkv::raft::InProcessCluster cluster({"node1", "node2", "node3"});
  expect(cluster.elect_leader("node1").ok_status(),
         "node1 should become leader");
  expect(cluster.set_available("node3", false).ok_status(),
         "node3 should be unavailable");

  const raftkv::Result<raftkv::raft::ReplicationResult> first =
      cluster.replicate_command(put_command("order:101", "paid"));
  const raftkv::Result<raftkv::raft::ReplicationResult> second =
      cluster.replicate_command(put_command("order:102", "shipped"));

  expect(first.ok() && first.value().committed,
         "first write should commit with node1 and node2");
  expect(second.ok() && second.value().committed,
         "second write should commit with node1 and node2");

  const raftkv::raft::RaftNode& offline = require_node(cluster, "node3");
  expect(offline.log().last_index() == 0,
         "offline follower should not receive entries");

  expect(cluster.set_available("node3", true).ok_status(),
         "node3 should be available again");
  const raftkv::Result<raftkv::raft::CatchUpResult> catch_up =
      cluster.sync_follower("node3");

  expect(catch_up.ok(), "catch-up should return result");
  expect(catch_up.value().caught_up, "node3 should catch up");
  expect(catch_up.value().from_index == 1,
         "node3 should start catch-up from first missing index");
  expect(catch_up.value().entries_sent == 2,
         "leader should send two missing entries");

  const raftkv::raft::RaftNode& follower = require_node(cluster, "node3");
  expect(follower.log().last_index() == 2,
         "caught-up follower should have both entries");
  expect(follower.log().commit_index() == 2,
         "caught-up follower should receive commit index");
  expect(follower.last_applied() == 2,
         "caught-up follower should apply committed entries");
  expect(follower.state_machine().get("order:101").value() == "paid",
         "caught-up follower should expose first committed value");
  expect(follower.state_machine().get("order:102").value() == "shipped",
         "caught-up follower should expose second committed value");
}

void test_new_leader_preserves_committed_data_after_failover() {
  raftkv::raft::InProcessCluster cluster({"node1", "node2", "node3"});
  expect(cluster.elect_leader("node1").ok_status(),
         "node1 should become leader");

  const raftkv::Result<raftkv::raft::ReplicationResult> first =
      cluster.replicate_command(put_command("session:1", "active"));
  expect(first.ok() && first.value().committed,
         "initial write should commit before failover");

  expect(cluster.set_available("node1", false).ok_status(),
         "old leader should become unavailable");
  expect(cluster.elect_leader("node2").ok_status(),
         "node2 should become new leader");
  expect(cluster.leader_id().has_value(), "cluster should record new leader");
  expect(*cluster.leader_id() == "node2", "new leader should be node2");

  const raftkv::raft::RaftNode& new_leader = require_node(cluster, "node2");
  expect(new_leader.state_machine().get("session:1").value() == "active",
         "new leader should retain committed value");

  const raftkv::Result<raftkv::raft::ReplicationResult> second =
      cluster.replicate_command(put_command("session:2", "active"));
  expect(second.ok() && second.value().committed,
         "cluster should accept writes after failover");
  expect(second.value().acknowledgements == 2,
         "new leader plus one follower should form majority");

  const raftkv::raft::RaftNode& follower = require_node(cluster, "node3");
  expect(follower.state_machine().get("session:1").value() == "active",
         "follower should retain pre-failover committed value");
  expect(follower.state_machine().get("session:2").value() == "active",
         "follower should apply post-failover committed value");
}

void test_stale_old_leader_cannot_commit_after_isolation() {
  raftkv::raft::InProcessCluster cluster({"node1", "node2", "node3"});
  expect(cluster.elect_leader("node1").ok_status(),
         "node1 should become leader");
  expect(cluster.set_available("node1", false).ok_status(),
         "old leader should be isolated");
  expect(cluster.elect_leader("node2").ok_status(),
         "majority side should elect node2");

  const raftkv::raft::RaftNode& old_leader = require_node(cluster, "node1");
  expect(old_leader.role() == raftkv::raft::RaftRole::kLeader,
         "isolated old leader may still believe it is leader");

  expect(cluster.leader_id().has_value(), "cluster should track active leader");
  expect(*cluster.leader_id() == "node2",
         "active majority leader should be node2");

  const raftkv::Result<raftkv::raft::ReplicationResult> write =
      cluster.replicate_command(put_command("split:test", "new-leader-write"));
  expect(write.ok() && write.value().committed,
         "new leader should commit with majority side");

  const raftkv::raft::RaftNode& new_leader = require_node(cluster, "node2");
  expect(new_leader.state_machine().get("split:test").value() ==
             "new-leader-write",
         "new leader value should win");
  expect(!old_leader.state_machine().get("split:test").ok(),
         "isolated old leader should not apply majority-side write");
}

}  // namespace

int main() {
  test_lagging_follower_catches_up_after_restart();
  test_new_leader_preserves_committed_data_after_failover();
  test_stale_old_leader_cannot_commit_after_isolation();

  std::cout << "PASS: failover tests\n";
  return 0;
}

