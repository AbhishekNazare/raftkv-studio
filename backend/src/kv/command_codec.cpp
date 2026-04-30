#include "raftkv/kv/command_codec.h"

#include <sstream>
#include <string_view>

namespace raftkv::kv {
namespace {

constexpr std::string_view kPutPrefix = "PUT";
constexpr std::string_view kDeletePrefix = "DELETE";

std::string escape_field(const std::string& value) {
  std::string escaped;
  escaped.reserve(value.size());

  for (const char ch : value) {
    if (ch == '\\' || ch == '\t' || ch == '\n') {
      escaped.push_back('\\');
      if (ch == '\t') {
        escaped.push_back('t');
      } else if (ch == '\n') {
        escaped.push_back('n');
      } else {
        escaped.push_back('\\');
      }
    } else {
      escaped.push_back(ch);
    }
  }

  return escaped;
}

Result<std::string> unescape_field(const std::string& value) {
  std::string unescaped;
  unescaped.reserve(value.size());

  for (std::size_t i = 0; i < value.size(); ++i) {
    const char ch = value[i];
    if (ch != '\\') {
      unescaped.push_back(ch);
      continue;
    }

    if (i + 1 >= value.size()) {
      return Status::error(StatusCode::kInvalidArgument,
                           "encoded command has dangling escape");
    }

    const char escaped = value[++i];
    if (escaped == 't') {
      unescaped.push_back('\t');
    } else if (escaped == 'n') {
      unescaped.push_back('\n');
    } else if (escaped == '\\') {
      unescaped.push_back('\\');
    } else {
      return Status::error(StatusCode::kInvalidArgument,
                           "encoded command has invalid escape");
    }
  }

  return unescaped;
}

}  // namespace

std::string encode_command(const Command& command) {
  std::ostringstream encoded;
  encoded << command_type_name(command.type) << '\t' << escape_field(command.key);

  if (command.type == CommandType::kPut) {
    encoded << '\t' << escape_field(command.value);
  }

  return encoded.str();
}

Result<Command> decode_command(const std::string& encoded) {
  const std::size_t first_separator = encoded.find('\t');
  if (first_separator == std::string::npos) {
    return Status::error(StatusCode::kInvalidArgument,
                         "encoded command is missing command type");
  }

  const std::string type = encoded.substr(0, first_separator);
  const std::string rest = encoded.substr(first_separator + 1);

  if (type == kDeletePrefix) {
    if (rest.find('\t') != std::string::npos) {
      return Status::error(StatusCode::kInvalidArgument,
                           "delete command must contain only a key");
    }

    Result<std::string> key = unescape_field(rest);
    if (!key.ok()) {
      return key.status();
    }

    Command command = Command::delete_key(key.value());
    const Status validation = validate_command(command);
    if (!validation.ok_status()) {
      return validation;
    }

    return command;
  }

  if (type != kPutPrefix) {
    return Status::error(StatusCode::kInvalidArgument,
                         "encoded command has unknown command type");
  }

  const std::size_t second_separator = rest.find('\t');
  if (second_separator == std::string::npos) {
    return Status::error(StatusCode::kInvalidArgument,
                         "put command is missing value");
  }

  const std::string encoded_key = rest.substr(0, second_separator);
  const std::string encoded_value = rest.substr(second_separator + 1);

  Result<std::string> key = unescape_field(encoded_key);
  if (!key.ok()) {
    return key.status();
  }

  Result<std::string> value = unescape_field(encoded_value);
  if (!value.ok()) {
    return value.status();
  }

  Command command = Command::put(key.value(), value.value());
  const Status validation = validate_command(command);
  if (!validation.ok_status()) {
    return validation;
  }

  return command;
}

}  // namespace raftkv::kv

