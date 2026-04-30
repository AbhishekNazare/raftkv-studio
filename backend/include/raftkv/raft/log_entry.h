#pragma once

#include <string>

#include "raftkv/common/types.h"

namespace raftkv::raft {

struct LogEntry {
  LogIndex index;
  Term term;
  std::string command;
};

}  // namespace raftkv::raft

