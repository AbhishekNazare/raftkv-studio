#pragma once

#include "raft.pb.h"
#include "raftkv/raft/log_entry.h"
#include "raftkv/raft/raft_node.h"

namespace raftkv::net {

rpc::v1::LogEntry to_proto(const raft::LogEntry& entry);
raft::LogEntry from_proto(const rpc::v1::LogEntry& entry);

rpc::v1::RequestVoteRequest to_proto(
    const raft::RequestVoteRequest& request);
raft::RequestVoteRequest from_proto(
    const rpc::v1::RequestVoteRequest& request);

rpc::v1::RequestVoteResponse to_proto(
    const raft::RequestVoteResponse& response);
raft::RequestVoteResponse from_proto(
    const rpc::v1::RequestVoteResponse& response);

rpc::v1::AppendEntriesRequest to_proto(
    const raft::AppendEntriesRequest& request);
raft::AppendEntriesRequest from_proto(
    const rpc::v1::AppendEntriesRequest& request);

rpc::v1::AppendEntriesResponse to_proto(
    const raft::AppendEntriesResponse& response);
raft::AppendEntriesResponse from_proto(
    const rpc::v1::AppendEntriesResponse& response);

}  // namespace raftkv::net

