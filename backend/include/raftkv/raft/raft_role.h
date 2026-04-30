#pragma once

#include <string>

namespace raftkv::raft {

enum class RaftRole {
  kFollower,
  kCandidate,
  kLeader,
};

std::string role_name(RaftRole role);

}  // namespace raftkv::raft

