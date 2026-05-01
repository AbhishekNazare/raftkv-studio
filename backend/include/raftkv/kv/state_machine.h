#pragma once

#include <map>
#include <optional>
#include <string>

#include "raftkv/common/result.h"
#include "raftkv/common/status.h"
#include "raftkv/kv/command.h"

namespace raftkv::kv {

class StateMachine {
 public:
  Status apply(const Command& command);
  Result<std::string> get(const std::string& key) const;
  [[nodiscard]] bool contains(const std::string& key) const;
  [[nodiscard]] std::size_t size() const;
  [[nodiscard]] std::map<std::string, std::string> snapshot() const;
  void replace_all(std::map<std::string, std::string> values);

 private:
  std::map<std::string, std::string> values_;
};

}  // namespace raftkv::kv
