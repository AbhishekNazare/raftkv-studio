#pragma once

#include <string>

#include "raftkv/common/status.h"

namespace raftkv::kv {

enum class CommandType {
  kPut,
  kDelete,
};

struct Command {
  CommandType type;
  std::string key;
  std::string value;

  static Command put(std::string key, std::string value);
  static Command delete_key(std::string key);
};

Status validate_command(const Command& command);
std::string command_type_name(CommandType type);

}  // namespace raftkv::kv

