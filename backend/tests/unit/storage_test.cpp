#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "raftkv/kv/command_codec.h"
#include "raftkv/storage/file_kv_store.h"
#include "raftkv/storage/file_log_store.h"
#include "raftkv/storage/file_metadata_store.h"

namespace {

void expect(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    std::exit(1);
  }
}

std::filesystem::path make_temp_dir() {
  const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
  std::filesystem::path path =
      std::filesystem::temp_directory_path() /
      ("raftkv-storage-test-" + std::to_string(now));
  std::filesystem::create_directories(path);
  return path;
}

std::string put_command(const std::string& key, const std::string& value) {
  return raftkv::kv::encode_command(raftkv::kv::Command::put(key, value));
}

void test_metadata_survives_store_reopen() {
  const std::filesystem::path dir = make_temp_dir();
  const std::filesystem::path path = dir / "metadata.txt";

  raftkv::storage::FileMetadataStore writer(path);
  expect(writer.save({7, std::string("node2")}).ok_status(),
         "metadata save should succeed");

  raftkv::storage::FileMetadataStore reader(path);
  const raftkv::Result<raftkv::storage::PersistentMetadata> loaded =
      reader.load();

  expect(loaded.ok(), "metadata load should succeed");
  expect(loaded.value().current_term == 7, "term should survive reload");
  expect(loaded.value().voted_for.has_value(), "vote should survive reload");
  expect(*loaded.value().voted_for == "node2", "vote should match saved node");

  std::filesystem::remove_all(dir);
}

void test_log_entries_survive_store_reopen() {
  const std::filesystem::path dir = make_temp_dir();
  const std::filesystem::path path = dir / "raft.log";

  raftkv::storage::FileLogStore writer(path);
  expect(writer.append({1, 2, put_command("user:1", "Abhishek")}).ok_status(),
         "first log append should persist");
  expect(writer.append({2, 2, put_command("city", "Gainesville")}).ok_status(),
         "second log append should persist");

  raftkv::storage::FileLogStore reader(path);
  const raftkv::Result<std::vector<raftkv::raft::LogEntry>> loaded =
      reader.load_all();

  expect(loaded.ok(), "log load should succeed");
  expect(loaded.value().size() == 2, "two log entries should load");
  expect(loaded.value()[0].index == 1, "first index should match");
  expect(loaded.value()[1].term == 2, "second term should match");
  expect(loaded.value()[1].command == put_command("city", "Gainesville"),
         "encoded command should survive reload");

  std::filesystem::remove_all(dir);
}

void test_log_overwrite_replaces_file_contents() {
  const std::filesystem::path dir = make_temp_dir();
  const std::filesystem::path path = dir / "raft.log";

  raftkv::storage::FileLogStore store(path);
  expect(store.append({1, 1, put_command("stale", "value")}).ok_status(),
         "initial append should persist");
  expect(store.overwrite_all({{1, 3, put_command("fresh", "value")},
                              {2, 3, put_command("fresh:2", "value")}})
             .ok_status(),
         "overwrite should persist replacement log");

  const raftkv::Result<std::vector<raftkv::raft::LogEntry>> loaded =
      store.load_all();

  expect(loaded.ok(), "overwritten log should load");
  expect(loaded.value().size() == 2, "replacement log should have two entries");
  expect(loaded.value()[0].term == 3, "replacement term should match");
  expect(loaded.value()[0].command == put_command("fresh", "value"),
         "stale command should be gone");

  std::filesystem::remove_all(dir);
}

void test_kv_snapshot_survives_store_reopen() {
  const std::filesystem::path dir = make_temp_dir();
  const std::filesystem::path path = dir / "kv.snapshot";

  raftkv::storage::FileKVStore writer(path);
  expect(writer.save_snapshot({{"line\tone", "hello\nworld"},
                               {"user:1", "Abhishek"}})
             .ok_status(),
         "kv snapshot save should succeed");

  raftkv::storage::FileKVStore reader(path);
  const raftkv::Result<std::map<std::string, std::string>> loaded =
      reader.load_snapshot();

  expect(loaded.ok(), "kv snapshot load should succeed");
  expect(loaded.value().size() == 2, "snapshot should load both keys");
  expect(loaded.value().at("user:1") == "Abhishek",
         "plain key should survive reload");
  expect(loaded.value().at("line\tone") == "hello\nworld",
         "escaped key and value should survive reload");

  std::filesystem::remove_all(dir);
}

void test_missing_files_load_as_empty_state() {
  const std::filesystem::path dir = make_temp_dir();

  raftkv::storage::FileMetadataStore metadata(dir / "missing-meta.txt");
  raftkv::storage::FileLogStore log(dir / "missing-raft.log");
  raftkv::storage::FileKVStore kv(dir / "missing-kv.snapshot");

  expect(metadata.load().ok(), "missing metadata should load default state");
  expect(metadata.load().value().current_term == 0,
         "missing metadata should default term to zero");
  expect(log.load_all().ok(), "missing log should load empty log");
  expect(log.load_all().value().empty(), "missing log should return no entries");
  expect(kv.load_snapshot().ok(), "missing kv snapshot should load empty state");
  expect(kv.load_snapshot().value().empty(),
         "missing kv snapshot should return no values");

  std::filesystem::remove_all(dir);
}

}  // namespace

int main() {
  test_metadata_survives_store_reopen();
  test_log_entries_survive_store_reopen();
  test_log_overwrite_replaces_file_contents();
  test_kv_snapshot_survives_store_reopen();
  test_missing_files_load_as_empty_state();

  std::cout << "PASS: storage tests\n";
  return 0;
}

