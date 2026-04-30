#include "raftkv/storage/file_metadata_store.h"

#include <filesystem>
#include <fstream>
#include <string>

namespace raftkv::storage {

FileMetadataStore::FileMetadataStore(std::filesystem::path path)
    : path_(std::move(path)) {}

Status FileMetadataStore::save(const PersistentMetadata& metadata) {
  std::error_code error;
  std::filesystem::create_directories(path_.parent_path(), error);
  if (error) {
    return Status::error(StatusCode::kInternal,
                         "failed to create metadata directory");
  }

  std::ofstream out(path_, std::ios::trunc);
  if (!out) {
    return Status::error(StatusCode::kInternal,
                         "failed to open metadata file for write");
  }

  out << metadata.current_term << '\n';
  out << (metadata.voted_for.has_value() ? *metadata.voted_for : "") << '\n';
  return Status::ok();
}

Result<PersistentMetadata> FileMetadataStore::load() const {
  if (!std::filesystem::exists(path_)) {
    return PersistentMetadata{};
  }

  std::ifstream in(path_);
  if (!in) {
    return Status::error(StatusCode::kInternal,
                         "failed to open metadata file for read");
  }

  PersistentMetadata metadata;
  std::string term_line;
  std::string voted_for_line;

  if (!std::getline(in, term_line)) {
    return Status::error(StatusCode::kInvalidArgument,
                         "metadata file is missing term");
  }

  try {
    metadata.current_term = static_cast<Term>(std::stoull(term_line));
  } catch (...) {
    return Status::error(StatusCode::kInvalidArgument,
                         "metadata term is invalid");
  }

  if (std::getline(in, voted_for_line) && !voted_for_line.empty()) {
    metadata.voted_for = voted_for_line;
  }

  return metadata;
}

}  // namespace raftkv::storage

