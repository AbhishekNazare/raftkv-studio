#include "raftkv/raft/in_process_cluster.h"

#include <utility>

namespace raftkv::raft {

InProcessCluster::InProcessCluster(std::vector<NodeId> node_ids)
    : node_order_(std::move(node_ids)) {
  for (const NodeId& id : node_order_) {
    nodes_.emplace(id, RaftNode(id));
    available_[id] = true;
  }
}

std::size_t InProcessCluster::size() const {
  return node_order_.size();
}

std::size_t InProcessCluster::majority() const {
  return (size() / 2) + 1;
}

const std::optional<NodeId>& InProcessCluster::leader_id() const {
  return leader_id_;
}

Result<RaftNode*> InProcessCluster::mutable_node(const NodeId& id) {
  auto found = nodes_.find(id);
  if (found == nodes_.end()) {
    return Status::error(StatusCode::kNotFound, "node not found");
  }

  return &found->second;
}

Result<const RaftNode*> InProcessCluster::node(const NodeId& id) const {
  auto found = nodes_.find(id);
  if (found == nodes_.end()) {
    return Status::error(StatusCode::kNotFound, "node not found");
  }

  return &found->second;
}

Status InProcessCluster::set_available(const NodeId& id, bool available) {
  if (!nodes_.contains(id)) {
    return Status::error(StatusCode::kNotFound, "node not found");
  }

  available_[id] = available;
  return Status::ok();
}

Status InProcessCluster::elect_leader(const NodeId& candidate_id) {
  Result<RaftNode*> candidate_result = mutable_node(candidate_id);
  if (!candidate_result.ok()) {
    return candidate_result.status();
  }

  if (!is_available(candidate_id)) {
    return Status::error(StatusCode::kUnavailable,
                         "candidate is unavailable");
  }

  RaftNode* candidate = candidate_result.value();
  candidate->start_election();

  std::size_t votes = 1;
  const RequestVoteRequest request = candidate->build_request_vote();

  for (const NodeId& peer_id : node_order_) {
    if (peer_id == candidate_id || !is_available(peer_id)) {
      continue;
    }

    RaftNode& peer = nodes_.at(peer_id);
    const RequestVoteResponse response = peer.handle_request_vote(request);
    if (response.term > candidate->current_term()) {
      candidate->become_follower(response.term, std::nullopt);
      return Status::error(StatusCode::kUnavailable,
                           "candidate observed newer term");
    }

    if (response.vote_granted) {
      ++votes;
    }
  }

  if (votes < majority()) {
    return Status::error(StatusCode::kUnavailable,
                         "candidate did not receive majority");
  }

  candidate->become_leader();
  leader_id_ = candidate_id;
  return Status::ok();
}

Result<ReplicationResult> InProcessCluster::replicate_command(
    std::string encoded_command) {
  Result<RaftNode*> leader_result = current_leader();
  if (!leader_result.ok()) {
    return leader_result.status();
  }

  if (!is_available(*leader_id_)) {
    return Status::error(StatusCode::kUnavailable, "leader is unavailable");
  }

  RaftNode* leader = leader_result.value();
  if (leader->role() != RaftRole::kLeader) {
    return Status::error(StatusCode::kUnavailable,
                         "recorded leader is not leader");
  }

  const LogEntry entry = leader->append_client_command(std::move(encoded_command));
  std::size_t acknowledgements = 1;

  for (const NodeId& peer_id : node_order_) {
    if (peer_id == *leader_id_ || !is_available(peer_id)) {
      continue;
    }

    RaftNode& peer = nodes_.at(peer_id);
    const LogIndex prev_index = entry.index - 1;
    const AppendEntriesRequest request{
        leader->current_term(),
        *leader_id_,
        prev_index,
        leader->log().term_at(prev_index),
        std::vector<LogEntry>{entry},
        leader->log().commit_index(),
    };

    const AppendEntriesResponse response = peer.handle_append_entries(request);
    if (response.term > leader->current_term()) {
      leader->become_follower(response.term, std::nullopt);
      leader_id_.reset();
      return Status::error(StatusCode::kUnavailable,
                           "leader observed newer term");
    }

    if (response.success) {
      ++acknowledgements;
    }
  }

  const bool committed = acknowledgements >= majority();
  if (committed) {
    const Status commit_status = leader->advance_commit_index(entry.index);
    if (!commit_status.ok_status()) {
      return commit_status;
    }
    broadcast_commit(entry.index);
  }

  return ReplicationResult{entry.index, acknowledgements, majority(), committed};
}

Result<CatchUpResult> InProcessCluster::sync_follower(
    const NodeId& follower_id) {
  Result<RaftNode*> leader_result = current_leader();
  if (!leader_result.ok()) {
    return leader_result.status();
  }

  if (!is_available(follower_id)) {
    return Status::error(StatusCode::kUnavailable, "follower is unavailable");
  }

  if (!nodes_.contains(follower_id)) {
    return Status::error(StatusCode::kNotFound, "follower not found");
  }

  if (leader_id_.has_value() && follower_id == *leader_id_) {
    return Status::error(StatusCode::kInvalidArgument,
                         "leader cannot catch up to itself");
  }

  RaftNode* leader = leader_result.value();
  RaftNode& follower = nodes_.at(follower_id);

  const LogIndex from_index = follower.log().last_index() + 1;
  std::size_t entries_sent = 0;

  for (const LogEntry& entry : leader->log().entries()) {
    if (entry.index < from_index) {
      continue;
    }

    const LogIndex prev_index = entry.index - 1;
    const AppendEntriesRequest request{
        leader->current_term(),
        *leader_id_,
        prev_index,
        follower.log().term_at(prev_index),
        std::vector<LogEntry>{entry},
        leader->log().commit_index(),
    };

    const AppendEntriesResponse response = follower.handle_append_entries(request);
    if (!response.success) {
      return Status::error(StatusCode::kUnavailable,
                           "follower rejected catch-up append");
    }

    ++entries_sent;
  }

  if (leader->log().commit_index() > follower.log().commit_index()) {
    const AppendEntriesRequest commit_request{
        leader->current_term(),
        *leader_id_,
        follower.log().last_index(),
        follower.log().last_term(),
        {},
        leader->log().commit_index(),
    };
    const AppendEntriesResponse response =
        follower.handle_append_entries(commit_request);
    if (!response.success) {
      return Status::error(StatusCode::kUnavailable,
                           "follower rejected catch-up commit");
    }
  }

  return CatchUpResult{
      follower_id,
      from_index,
      leader->log().last_index(),
      entries_sent,
      follower.log().last_index() == leader->log().last_index() &&
          follower.log().commit_index() == leader->log().commit_index(),
  };
}

bool InProcessCluster::is_available(const NodeId& id) const {
  const auto found = available_.find(id);
  return found != available_.end() && found->second;
}

Result<RaftNode*> InProcessCluster::current_leader() {
  if (!leader_id_.has_value()) {
    return Status::error(StatusCode::kUnavailable, "cluster has no leader");
  }

  Result<RaftNode*> leader_result = mutable_node(*leader_id_);
  if (!leader_result.ok()) {
    return leader_result.status();
  }

  if (!is_available(*leader_id_)) {
    return Status::error(StatusCode::kUnavailable, "leader is unavailable");
  }

  if (leader_result.value()->role() != RaftRole::kLeader) {
    return Status::error(StatusCode::kUnavailable,
                         "recorded leader is not leader");
  }

  return leader_result;
}

Result<const RaftNode*> InProcessCluster::current_leader() const {
  if (!leader_id_.has_value()) {
    return Status::error(StatusCode::kUnavailable, "cluster has no leader");
  }

  Result<const RaftNode*> leader_result = node(*leader_id_);
  if (!leader_result.ok()) {
    return leader_result.status();
  }

  if (!is_available(*leader_id_)) {
    return Status::error(StatusCode::kUnavailable, "leader is unavailable");
  }

  if (leader_result.value()->role() != RaftRole::kLeader) {
    return Status::error(StatusCode::kUnavailable,
                         "recorded leader is not leader");
  }

  return leader_result;
}

void InProcessCluster::broadcast_commit(LogIndex commit_index) {
  if (!leader_id_.has_value()) {
    return;
  }

  const RaftNode& leader = nodes_.at(*leader_id_);
  for (const NodeId& peer_id : node_order_) {
    if (peer_id == *leader_id_ || !is_available(peer_id)) {
      continue;
    }

    RaftNode& peer = nodes_.at(peer_id);
    const AppendEntriesRequest request{
        leader.current_term(),
        *leader_id_,
        peer.log().last_index(),
        peer.log().last_term(),
        {},
        commit_index,
    };
    peer.handle_append_entries(request);
  }
}

}  // namespace raftkv::raft
