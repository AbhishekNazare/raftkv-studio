#include "raftkv/net/raft_rpc_mapper.h"

namespace raftkv::net {

rpc::v1::LogEntry to_proto(const raft::LogEntry& entry) {
  rpc::v1::LogEntry proto;
  proto.set_index(entry.index);
  proto.set_term(entry.term);
  proto.set_command(entry.command);
  return proto;
}

raft::LogEntry from_proto(const rpc::v1::LogEntry& entry) {
  return raft::LogEntry{
      entry.index(),
      entry.term(),
      entry.command(),
  };
}

rpc::v1::RequestVoteRequest to_proto(
    const raft::RequestVoteRequest& request) {
  rpc::v1::RequestVoteRequest proto;
  proto.set_term(request.term);
  proto.set_candidate_id(request.candidate_id);
  proto.set_last_log_index(request.last_log_index);
  proto.set_last_log_term(request.last_log_term);
  return proto;
}

raft::RequestVoteRequest from_proto(
    const rpc::v1::RequestVoteRequest& request) {
  return raft::RequestVoteRequest{
      request.term(),
      request.candidate_id(),
      request.last_log_index(),
      request.last_log_term(),
  };
}

rpc::v1::RequestVoteResponse to_proto(
    const raft::RequestVoteResponse& response) {
  rpc::v1::RequestVoteResponse proto;
  proto.set_term(response.term);
  proto.set_vote_granted(response.vote_granted);
  return proto;
}

raft::RequestVoteResponse from_proto(
    const rpc::v1::RequestVoteResponse& response) {
  return raft::RequestVoteResponse{
      response.term(),
      response.vote_granted(),
  };
}

rpc::v1::AppendEntriesRequest to_proto(
    const raft::AppendEntriesRequest& request) {
  rpc::v1::AppendEntriesRequest proto;
  proto.set_term(request.term);
  proto.set_leader_id(request.leader_id);
  proto.set_prev_log_index(request.prev_log_index);
  proto.set_prev_log_term(request.prev_log_term);
  proto.set_leader_commit(request.leader_commit);

  for (const raft::LogEntry& entry : request.entries) {
    *proto.add_entries() = to_proto(entry);
  }

  return proto;
}

raft::AppendEntriesRequest from_proto(
    const rpc::v1::AppendEntriesRequest& request) {
  raft::AppendEntriesRequest converted{
      request.term(),
      request.leader_id(),
      request.prev_log_index(),
      request.prev_log_term(),
      {},
      request.leader_commit(),
  };

  converted.entries.reserve(
      static_cast<std::size_t>(request.entries_size()));
  for (const rpc::v1::LogEntry& entry : request.entries()) {
    converted.entries.push_back(from_proto(entry));
  }

  return converted;
}

rpc::v1::AppendEntriesResponse to_proto(
    const raft::AppendEntriesResponse& response) {
  rpc::v1::AppendEntriesResponse proto;
  proto.set_term(response.term);
  proto.set_success(response.success);
  return proto;
}

raft::AppendEntriesResponse from_proto(
    const rpc::v1::AppendEntriesResponse& response) {
  return raft::AppendEntriesResponse{
      response.term(),
      response.success(),
  };
}

}  // namespace raftkv::net

