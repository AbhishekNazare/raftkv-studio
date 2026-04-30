#include "raftkv/kv/state_machine.h"

#include <utility>

namespace raftkv::kv {

Status StateMachine::apply(const Command& command) {
  const Status validation = validate_command(command);
  if (!validation.ok_status()) {
    return validation;
  }

  switch (command.type) {
    case CommandType::kPut:
      values_[command.key] = command.value;
      return Status::ok();
    case CommandType::kDelete:
      values_.erase(command.key);
      return Status::ok();
  }

  return Status::error(StatusCode::kInvalidArgument, "unknown command type");
}

Result<std::string> StateMachine::get(const std::string& key) const {
  const auto found = values_.find(key);
  if (found == values_.end()) {
    return Status::error(StatusCode::kNotFound, "key not found");
  }

  return found->second;
}

bool StateMachine::contains(const std::string& key) const {
  return values_.contains(key);
}

std::size_t StateMachine::size() const {
  return values_.size();
}

std::map<std::string, std::string> StateMachine::snapshot() const {
  return values_;
}

}  // namespace raftkv::kv

