#include <cstdlib>
#include <iostream>
#include <string>

#include "raftkv/kv/command_codec.h"
#include "raftkv/kv/state_machine.h"

namespace {

void expect(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    std::exit(1);
  }
}

void test_put_get_delete() {
  raftkv::kv::StateMachine state;

  raftkv::Status status = state.apply(
      raftkv::kv::Command::put("user:1", "Abhishek"));
  expect(status.ok_status(), "put should apply successfully");
  expect(state.size() == 1, "put should add one key");

  raftkv::Result<std::string> value = state.get("user:1");
  expect(value.ok(), "stored key should be readable");
  expect(value.value() == "Abhishek", "stored value should match put value");

  status = state.apply(raftkv::kv::Command::delete_key("user:1"));
  expect(status.ok_status(), "delete should apply successfully");
  expect(!state.contains("user:1"), "delete should remove key");

  value = state.get("user:1");
  expect(!value.ok(), "deleted key should not be found");
  expect(value.status().code() == raftkv::StatusCode::kNotFound,
         "deleted key should return not found");
}

void test_put_overwrites_deterministically() {
  raftkv::kv::StateMachine state;

  expect(state.apply(raftkv::kv::Command::put("city", "Gainesville"))
             .ok_status(),
         "first put should apply");
  expect(state.apply(raftkv::kv::Command::put("city", "Seattle")).ok_status(),
         "second put should apply");

  const raftkv::Result<std::string> value = state.get("city");
  expect(value.ok(), "overwritten key should be readable");
  expect(value.value() == "Seattle", "last applied put should win");
  expect(state.size() == 1, "overwriting should not create duplicate keys");
}

void test_delete_is_idempotent_for_missing_key() {
  raftkv::kv::StateMachine state;

  const raftkv::Status status =
      state.apply(raftkv::kv::Command::delete_key("missing"));
  expect(status.ok_status(), "deleting a missing key should be harmless");
  expect(state.size() == 0, "delete missing key should keep state empty");
}

void test_invalid_command_is_rejected() {
  raftkv::kv::StateMachine state;

  const raftkv::Status status =
      state.apply(raftkv::kv::Command::put("", "value"));
  expect(!status.ok_status(), "empty key should be rejected");
  expect(status.code() == raftkv::StatusCode::kInvalidArgument,
         "empty key should be invalid argument");
  expect(state.size() == 0, "invalid command should not mutate state");
}

void test_command_codec_round_trip() {
  const raftkv::kv::Command command =
      raftkv::kv::Command::put("line\tone", "hello\nworld\\done");

  const std::string encoded = raftkv::kv::encode_command(command);
  const raftkv::Result<raftkv::kv::Command> decoded =
      raftkv::kv::decode_command(encoded);

  expect(decoded.ok(), "encoded put command should decode");
  expect(decoded.value().type == raftkv::kv::CommandType::kPut,
         "decoded command should be put");
  expect(decoded.value().key == command.key, "decoded key should match");
  expect(decoded.value().value == command.value, "decoded value should match");

  const raftkv::kv::Command delete_command =
      raftkv::kv::Command::delete_key("user:1");
  const raftkv::Result<raftkv::kv::Command> decoded_delete =
      raftkv::kv::decode_command(raftkv::kv::encode_command(delete_command));

  expect(decoded_delete.ok(), "encoded delete command should decode");
  expect(decoded_delete.value().type == raftkv::kv::CommandType::kDelete,
         "decoded command should be delete");
  expect(decoded_delete.value().key == "user:1",
         "decoded delete key should match");
}

}  // namespace

int main() {
  test_put_get_delete();
  test_put_overwrites_deterministically();
  test_delete_is_idempotent_for_missing_key();
  test_invalid_command_is_rejected();
  test_command_codec_round_trip();

  std::cout << "PASS: state machine tests\n";
  return 0;
}

