#pragma once

#include <utility>
#include <variant>

#include "raftkv/common/status.h"

namespace raftkv {

template <typename T>
class Result {
 public:
  Result(T value) : value_(std::move(value)) {}
  Result(Status status) : value_(std::move(status)) {}

  [[nodiscard]] bool ok() const {
    return std::holds_alternative<T>(value_);
  }

  [[nodiscard]] const T& value() const {
    return std::get<T>(value_);
  }

  [[nodiscard]] const Status& status() const {
    return std::get<Status>(value_);
  }

 private:
  std::variant<T, Status> value_;
};

}  // namespace raftkv

