#include "raftkv/storage/file_kv_store.h"

#include <filesystem>
#include <fstream>
#include <map>
#include <string>

#include "raftkv/kv/command_codec.h"

namespace raftkv::storage {

FileKVStore::FileKVStore(std::filesystem::path path) : path_(std::move(path)) {}

Status FileKVStore::save_snapshot(
    const std::map<std::string, std::string>& values) {
  std::error_code error;
  std::filesystem::create_directories(path_.parent_path(), error);
  if (error) {
    return Status::error(StatusCode::kInternal,
                         "failed to create kv directory");
  }

  std::ofstream out(path_, std::ios::trunc);
  if (!out) {
    return Status::error(StatusCode::kInternal,
                         "failed to open kv file for write");
  }

  for (const auto& [key, value] : values) {
    out << kv::encode_command(kv::Command::put(key, value)) << '\n';
  }

  return Status::ok();
}

Result<std::map<std::string, std::string>> FileKVStore::load_snapshot() const {
  std::map<std::string, std::string> values;
  if (!std::filesystem::exists(path_)) {
    return values;
  }

  std::ifstream in(path_);
  if (!in) {
    return Status::error(StatusCode::kInternal,
                         "failed to open kv file for read");
  }

  std::string line;
  while (std::getline(in, line)) {
    if (line.empty()) {
      continue;
    }

    Result<kv::Command> command = kv::decode_command(line);
    if (!command.ok()) {
      return command.status();
    }

    if (command.value().type != kv::CommandType::kPut) {
      return Status::error(StatusCode::kInvalidArgument,
                           "kv snapshot only supports put commands");
    }

    values[command.value().key] = command.value().value;
  }

  return values;
}

}  // namespace raftkv::storage

