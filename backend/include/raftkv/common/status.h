#pragma once

#include <string>

namespace raftkv {

enum class StatusCode {
  kOk,
  kInvalidArgument,
  kNotFound,
  kUnavailable,
  kInternal,
};

class Status {
 public:
  Status() = default;

  static Status ok() { return Status(); }

  static Status error(StatusCode code, std::string message) {
    return Status(code, std::move(message));
  }

  [[nodiscard]] bool ok_status() const { return code_ == StatusCode::kOk; }
  [[nodiscard]] StatusCode code() const { return code_; }
  [[nodiscard]] const std::string& message() const { return message_; }

 private:
  Status(StatusCode code, std::string message)
      : code_(code), message_(std::move(message)) {}

  StatusCode code_{StatusCode::kOk};
  std::string message_;
};

}  // namespace raftkv

