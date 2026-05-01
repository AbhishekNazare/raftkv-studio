#include <cstdlib>
#include <iostream>
#include <string>

#include "raftkv/kv/command_codec.h"
#include "raftkv/raft/in_process_cluster.h"
#include "raftkv/raft/raft_log.h"

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

void test_log_compaction_keeps_snapshot_boundary_term() {
  raftkv::raft::RaftLog log;
  log.append(1, "entry-1");
  log.append(2, "entry-2");
  log.append(2, "entry-3");

  expect(log.advance_commit_index(2).ok_status(),
         "commit index should advance before compaction");
  expect(log.compact_up_to(2).ok_status(), "compaction should succeed");

  expect(log.snapshot_metadata().last_included_index == 2,
         "snapshot boundary index should be recorded");
  expect(log.snapshot_metadata().last_included_term == 2,
         "snapshot boundary term should be recorded");
  expect(log.term_at(2) == 2, "term lookup should work at snapshot boundary");
  expect(log.matches_at(2, 2), "snapshot boundary should match append prev");
  expect(log.size() == 1, "entries covered by snapshot should be removed");
  expect(log.entry_at(1).status().code() == raftkv::StatusCode::kNotFound,
         "compacted entry should no longer be addressable");
}

void test_leader_snapshot_compacts_committed_log() {
  raftkv::raft::InProcessCluster cluster({"node1", "node2", "node3"});
  expect(cluster.elect_leader("node1").ok_status(),
         "node1 should become leader");
  expect(cluster.replicate_command(put_command("key:1", "value:1")).value().committed,
         "first write should commit");
  expect(cluster.replicate_command(put_command("key:2", "value:2")).value().committed,
         "second write should commit");

  const raftkv::Result<raftkv::raft::Snapshot> snapshot =
      cluster.create_leader_snapshot();

  expect(snapshot.ok(), "leader snapshot should be created");
  expect(snapshot.value().metadata.last_included_index == 2,
         "snapshot should cover committed entries");
  expect(snapshot.value().kv_state.at("key:1") == "value:1",
         "snapshot should include first key");
  expect(snapshot.value().kv_state.at("key:2") == "value:2",
         "snapshot should include second key");

  const raftkv::raft::RaftNode& leader = require_node(cluster, "node1");
  expect(leader.log().snapshot_metadata().last_included_index == 2,
         "leader log should record compacted snapshot index");
  expect(leader.log().entries().empty(),
         "leader log should compact entries covered by snapshot");
}

void test_far_behind_follower_installs_snapshot() {
  raftkv::raft::InProcessCluster cluster({"node1", "node2", "node3"});
  expect(cluster.elect_leader("node1").ok_status(),
         "node1 should become leader");
  expect(cluster.set_available("node3", false).ok_status(),
         "node3 should be offline");

  expect(cluster.replicate_command(put_command("key:1", "value:1")).value().committed,
         "first write should commit");
  expect(cluster.replicate_command(put_command("key:2", "value:2")).value().committed,
         "second write should commit");
  expect(cluster.create_leader_snapshot().ok(),
         "leader should create and compact snapshot");

  expect(cluster.set_available("node3", true).ok_status(),
         "node3 should come back");
  const raftkv::Result<raftkv::raft::CatchUpResult> catch_up =
      cluster.sync_follower("node3");

  expect(catch_up.ok(), "catch-up should succeed");
  expect(catch_up.value().snapshot_installed,
         "far-behind follower should install snapshot");
  expect(catch_up.value().caught_up, "follower should be caught up");

  const raftkv::raft::RaftNode& follower = require_node(cluster, "node3");
  expect(follower.log().snapshot_metadata().last_included_index == 2,
         "follower should record installed snapshot metadata");
  expect(follower.last_applied() == 2,
         "follower should advance last applied to snapshot boundary");
  expect(follower.state_machine().get("key:1").value() == "value:1",
         "follower should restore first key from snapshot");
  expect(follower.state_machine().get("key:2").value() == "value:2",
         "follower should restore second key from snapshot");
}

}  // namespace

int main() {
  test_log_compaction_keeps_snapshot_boundary_term();
  test_leader_snapshot_compacts_committed_log();
  test_far_behind_follower_installs_snapshot();

  std::cout << "PASS: snapshot tests\n";
  return 0;
}

