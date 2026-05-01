#pragma once

#include <optional>
#include <string>
#include <vector>

#include "raftkv/common/types.h"
#include "raftkv/kv/state_machine.h"
#include "raftkv/raft/raft_log.h"
#include "raftkv/raft/raft_role.h"

namespace raftkv::raft {

struct RequestVoteRequest {
  Term term;
  NodeId candidate_id;
  LogIndex last_log_index;
  Term last_log_term;
};

struct RequestVoteResponse {
  Term term;
  bool vote_granted;
};

struct AppendEntriesRequest {
  Term term;
  NodeId leader_id;
  LogIndex prev_log_index;
  Term prev_log_term;
  std::vector<LogEntry> entries;
  LogIndex leader_commit;
};

struct AppendEntriesResponse {
  Term term;
  bool success;
};

class RaftNode {
 public:
  explicit RaftNode(NodeId id);

  [[nodiscard]] const NodeId& id() const;
  [[nodiscard]] RaftRole role() const;
  [[nodiscard]] Term current_term() const;
  [[nodiscard]] const std::optional<NodeId>& voted_for() const;
  [[nodiscard]] const std::optional<NodeId>& leader_id() const;
  [[nodiscard]] const RaftLog& log() const;
  [[nodiscard]] RaftLog& mutable_log();
  [[nodiscard]] const kv::StateMachine& state_machine() const;
  [[nodiscard]] LogIndex last_applied() const;
  [[nodiscard]] const std::optional<Snapshot>& latest_snapshot() const;

  void become_follower(Term term, std::optional<NodeId> leader_id);
  void start_election();
  void become_leader();

  RequestVoteRequest build_request_vote() const;
  RequestVoteResponse handle_request_vote(const RequestVoteRequest& request);
  AppendEntriesResponse handle_append_entries(
      const AppendEntriesRequest& request);
  LogEntry append_client_command(std::string encoded_command);
  Status advance_commit_index(LogIndex new_commit_index);
  Result<Snapshot> create_snapshot();
  Status install_snapshot(const Snapshot& snapshot);

 private:
  [[nodiscard]] bool candidate_log_is_at_least_as_fresh(
      const RequestVoteRequest& request) const;
  Status apply_committed_entries();
  void step_down_to_term(Term term);

  NodeId id_;
  RaftRole role_{RaftRole::kFollower};
  Term current_term_{0};
  std::optional<NodeId> voted_for_;
  std::optional<NodeId> leader_id_;
  RaftLog log_;
  kv::StateMachine state_machine_;
  LogIndex last_applied_{0};
  std::optional<Snapshot> latest_snapshot_;
};

}  // namespace raftkv::raft
