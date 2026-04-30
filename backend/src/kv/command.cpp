#include "raftkv/kv/command.h"

#include <utility>

namespace raftkv::kv {

Command Command::put(std::string key, std::string value) {
  return Command{CommandType::kPut, std::move(key), std::move(value)};
}

Command Command::delete_key(std::string key) {
  return Command{CommandType::kDelete, std::move(key), ""};
}

Status validate_command(const Command& command) {
  if (command.key.empty()) {
    return Status::error(StatusCode::kInvalidArgument,
                         "command key must not be empty");
  }

  return Status::ok();
}

std::string command_type_name(CommandType type) {
  switch (type) {
    case CommandType::kPut:
      return "PUT";
    case CommandType::kDelete:
      return "DELETE";
  }

  return "UNKNOWN";
}

}  // namespace raftkv::kv

