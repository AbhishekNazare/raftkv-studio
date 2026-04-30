#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "raftkv/common/result.h"
#include "raftkv/common/status.h"
#include "raftkv/common/types.h"
#include "raftkv/raft/raft_node.h"

namespace raftkv::raft {

struct ReplicationResult {
  LogIndex index;
  std::size_t acknowledgements;
  std::size_t majority;
  bool committed;
};

class InProcessCluster {
 public:
  explicit InProcessCluster(std::vector<NodeId> node_ids);

  [[nodiscard]] std::size_t size() const;
  [[nodiscard]] std::size_t majority() const;
  [[nodiscard]] const std::optional<NodeId>& leader_id() const;

  Result<RaftNode*> mutable_node(const NodeId& id);
  Result<const RaftNode*> node(const NodeId& id) const;

  Status set_available(const NodeId& id, bool available);
  Status elect_leader(const NodeId& candidate_id);
  Result<ReplicationResult> replicate_command(std::string encoded_command);

 private:
  [[nodiscard]] bool is_available(const NodeId& id) const;
  void broadcast_commit(LogIndex commit_index);

  std::vector<NodeId> node_order_;
  std::map<NodeId, RaftNode> nodes_;
  std::map<NodeId, bool> available_;
  std::optional<NodeId> leader_id_;
};

}  // namespace raftkv::raft
