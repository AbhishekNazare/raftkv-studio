#include "raftkv/storage/file_log_store.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace raftkv::storage {
namespace {

Status ensure_parent_directory(const std::filesystem::path& path) {
  std::error_code error;
  std::filesystem::create_directories(path.parent_path(), error);
  if (error) {
    return Status::error(StatusCode::kInternal,
                         "failed to create log directory");
  }

  return Status::ok();
}

Result<raft::LogEntry> parse_log_line(const std::string& line) {
  const std::size_t first = line.find('\t');
  if (first == std::string::npos) {
    return Status::error(StatusCode::kInvalidArgument,
                         "log line is missing index separator");
  }

  const std::size_t second = line.find('\t', first + 1);
  if (second == std::string::npos) {
    return Status::error(StatusCode::kInvalidArgument,
                         "log line is missing term separator");
  }

  try {
    const LogIndex index = static_cast<LogIndex>(
        std::stoull(line.substr(0, first)));
    const Term term = static_cast<Term>(
        std::stoull(line.substr(first + 1, second - first - 1)));
    const std::string command = line.substr(second + 1);
    return raft::LogEntry{index, term, command};
  } catch (...) {
    return Status::error(StatusCode::kInvalidArgument,
                         "log line has invalid numeric field");
  }
}

}  // namespace

FileLogStore::FileLogStore(std::filesystem::path path) : path_(std::move(path)) {}

Status FileLogStore::append(const raft::LogEntry& entry) {
  const Status directory_status = ensure_parent_directory(path_);
  if (!directory_status.ok_status()) {
    return directory_status;
  }

  std::ofstream out(path_, std::ios::app);
  if (!out) {
    return Status::error(StatusCode::kInternal,
                         "failed to open log file for append");
  }

  out << entry.index << '\t' << entry.term << '\t' << entry.command << '\n';
  return Status::ok();
}

Status FileLogStore::overwrite_all(const std::vector<raft::LogEntry>& entries) {
  const Status directory_status = ensure_parent_directory(path_);
  if (!directory_status.ok_status()) {
    return directory_status;
  }

  std::ofstream out(path_, std::ios::trunc);
  if (!out) {
    return Status::error(StatusCode::kInternal,
                         "failed to open log file for write");
  }

  for (const raft::LogEntry& entry : entries) {
    out << entry.index << '\t' << entry.term << '\t' << entry.command << '\n';
  }

  return Status::ok();
}

Result<std::vector<raft::LogEntry>> FileLogStore::load_all() const {
  std::vector<raft::LogEntry> entries;
  if (!std::filesystem::exists(path_)) {
    return entries;
  }

  std::ifstream in(path_);
  if (!in) {
    return Status::error(StatusCode::kInternal,
                         "failed to open log file for read");
  }

  std::string line;
  LogIndex expected_index = 1;
  while (std::getline(in, line)) {
    if (line.empty()) {
      continue;
    }

    Result<raft::LogEntry> entry = parse_log_line(line);
    if (!entry.ok()) {
      return entry.status();
    }

    if (entry.value().index != expected_index) {
      return Status::error(StatusCode::kInvalidArgument,
                           "log file has non-contiguous indexes");
    }

    entries.push_back(entry.value());
    ++expected_index;
  }

  return entries;
}

}  // namespace raftkv::storage

