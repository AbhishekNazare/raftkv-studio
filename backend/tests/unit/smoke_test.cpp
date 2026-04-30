#include <cstdlib>
#include <iostream>
#include <string>

#include "raftkv/common/result.h"
#include "raftkv/common/status.h"
#include "raftkv/common/types.h"

namespace {

void expect(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    std::exit(1);
  }
}

}  // namespace

int main() {
  const raftkv::Version version = raftkv::backend_version();
  expect(version.major == 0, "major version should start at 0");
  expect(version.minor == 1, "minor version should start at 1");
  expect(version.patch == 0, "patch version should start at 0");
  expect(raftkv::backend_version_string() == "0.1.0",
         "version string should be 0.1.0");

  const raftkv::Status ok = raftkv::Status::ok();
  expect(ok.ok_status(), "default status should be ok");

  const raftkv::Result<int> result(42);
  expect(result.ok(), "result with value should be ok");
  expect(result.value() == 42, "result should expose stored value");

  std::cout << "PASS: backend smoke test\n";
  return 0;
}

