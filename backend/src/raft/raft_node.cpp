#include "raftkv/raft/raft_node.h"

#include <utility>

#include "raftkv/kv/command_codec.h"

namespace raftkv::raft {

std::string role_name(RaftRole role) {
  switch (role) {
    case RaftRole::kFollower:
      return "FOLLOWER";
    case RaftRole::kCandidate:
      return "CANDIDATE";
    case RaftRole::kLeader:
      return "LEADER";
  }

  return "UNKNOWN";
}

RaftNode::RaftNode(NodeId id) : id_(std::move(id)) {}

const NodeId& RaftNode::id() const {
  return id_;
}

RaftRole RaftNode::role() const {
  return role_;
}

Term RaftNode::current_term() const {
  return current_term_;
}

const std::optional<NodeId>& RaftNode::voted_for() const {
  return voted_for_;
}

const std::optional<NodeId>& RaftNode::leader_id() const {
  return leader_id_;
}

const RaftLog& RaftNode::log() const {
  return log_;
}

RaftLog& RaftNode::mutable_log() {
  return log_;
}

const kv::StateMachine& RaftNode::state_machine() const {
  return state_machine_;
}

LogIndex RaftNode::last_applied() const {
  return last_applied_;
}

const std::optional<Snapshot>& RaftNode::latest_snapshot() const {
  return latest_snapshot_;
}

void RaftNode::become_follower(Term term, std::optional<NodeId> leader_id) {
  role_ = RaftRole::kFollower;
  current_term_ = term;
  leader_id_ = std::move(leader_id);
  voted_for_.reset();
}

void RaftNode::start_election() {
  role_ = RaftRole::kCandidate;
  ++current_term_;
  voted_for_ = id_;
  leader_id_.reset();
}

void RaftNode::become_leader() {
  role_ = RaftRole::kLeader;
  leader_id_ = id_;
}

RequestVoteRequest RaftNode::build_request_vote() const {
  return RequestVoteRequest{
      current_term_,
      id_,
      log_.last_index(),
      log_.last_term(),
  };
}

RequestVoteResponse RaftNode::handle_request_vote(
    const RequestVoteRequest& request) {
  if (request.term < current_term_) {
    return RequestVoteResponse{current_term_, false};
  }

  if (request.term > current_term_) {
    step_down_to_term(request.term);
  }

  const bool already_voted_for_candidate =
      voted_for_.has_value() && *voted_for_ == request.candidate_id;
  const bool vote_available =
      !voted_for_.has_value() || already_voted_for_candidate;

  if (vote_available && candidate_log_is_at_least_as_fresh(request)) {
    voted_for_ = request.candidate_id;
    leader_id_.reset();
    role_ = RaftRole::kFollower;
    return RequestVoteResponse{current_term_, true};
  }

  return RequestVoteResponse{current_term_, false};
}

AppendEntriesResponse RaftNode::handle_append_entries(
    const AppendEntriesRequest& request) {
  if (request.term < current_term_) {
    return AppendEntriesResponse{current_term_, false};
  }

  if (request.term > current_term_) {
    step_down_to_term(request.term);
  }

  role_ = RaftRole::kFollower;
  leader_id_ = request.leader_id;

  if (!log_.matches_at(request.prev_log_index, request.prev_log_term)) {
    return AppendEntriesResponse{current_term_, false};
  }

  const Status append_status = log_.append_from_leader(
      request.prev_log_index, request.prev_log_term, request.entries);
  if (!append_status.ok_status()) {
    return AppendEntriesResponse{current_term_, false};
  }

  if (request.leader_commit > log_.commit_index()) {
    const LogIndex new_commit_index =
        request.leader_commit < log_.last_index() ? request.leader_commit
                                                  : log_.last_index();
    const Status status = advance_commit_index(new_commit_index);
    if (!status.ok_status()) {
      return AppendEntriesResponse{current_term_, false};
    }
  }

  return AppendEntriesResponse{current_term_, true};
}

LogEntry RaftNode::append_client_command(std::string encoded_command) {
  return log_.append(current_term_, std::move(encoded_command));
}

Status RaftNode::advance_commit_index(LogIndex new_commit_index) {
  const Status status = log_.advance_commit_index(new_commit_index);
  if (!status.ok_status()) {
    return status;
  }

  return apply_committed_entries();
}

Result<Snapshot> RaftNode::create_snapshot() {
  if (log_.commit_index() == 0) {
    return Status::error(StatusCode::kInvalidArgument,
                         "cannot snapshot empty committed state");
  }

  const SnapshotMetadata metadata{
      log_.commit_index(),
      log_.term_at(log_.commit_index()),
  };
  if (metadata.last_included_term == 0) {
    return Status::error(StatusCode::kNotFound,
                         "committed snapshot boundary is missing");
  }

  Snapshot snapshot{metadata, state_machine_.snapshot()};
  const Status compact_status = log_.compact_up_to(metadata.last_included_index);
  if (!compact_status.ok_status()) {
    return compact_status;
  }

  latest_snapshot_ = snapshot;
  return snapshot;
}

Status RaftNode::install_snapshot(const Snapshot& snapshot) {
  const Status status = log_.install_snapshot_metadata(snapshot.metadata);
  if (!status.ok_status()) {
    return status;
  }

  state_machine_.replace_all(snapshot.kv_state);
  last_applied_ = snapshot.metadata.last_included_index;
  latest_snapshot_ = snapshot;
  return Status::ok();
}

bool RaftNode::candidate_log_is_at_least_as_fresh(
    const RequestVoteRequest& request) const {
  if (request.last_log_term != log_.last_term()) {
    return request.last_log_term > log_.last_term();
  }

  return request.last_log_index >= log_.last_index();
}

Status RaftNode::apply_committed_entries() {
  while (last_applied_ < log_.commit_index()) {
    const LogIndex next_index = last_applied_ + 1;
    Result<LogEntry> entry = log_.entry_at(next_index);
    if (!entry.ok()) {
      return entry.status();
    }

    Result<kv::Command> command = kv::decode_command(entry.value().command);
    if (!command.ok()) {
      return command.status();
    }

    const Status apply_status = state_machine_.apply(command.value());
    if (!apply_status.ok_status()) {
      return apply_status;
    }

    last_applied_ = next_index;
  }

  return Status::ok();
}

void RaftNode::step_down_to_term(Term term) {
  current_term_ = term;
  role_ = RaftRole::kFollower;
  voted_for_.reset();
  leader_id_.reset();
}

}  // namespace raftkv::raft
