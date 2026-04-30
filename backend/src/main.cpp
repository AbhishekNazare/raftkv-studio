#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "raftkv/common/types.h"

namespace {

std::atomic_bool shutdown_requested{false};

void handle_signal(int) {
  shutdown_requested = true;
}

std::string arg_value(const std::vector<std::string>& args,
                      const std::string& name,
                      const std::string& fallback) {
  for (std::size_t i = 0; i + 1 < args.size(); ++i) {
    if (args[i] == name) {
      return args[i + 1];
    }
  }

  return fallback;
}

bool has_flag(const std::vector<std::string>& args, const std::string& name) {
  for (const std::string& arg : args) {
    if (arg == name) {
      return true;
    }
  }

  return false;
}

}  // namespace

int main(int argc, char** argv) {
  const std::vector<std::string> args(argv + 1, argv + argc);

  if (has_flag(args, "--version")) {
    std::cout << "raftkv-node " << raftkv::backend_version_string() << "\n";
    return 0;
  }

  const std::string node_id = arg_value(args, "--node-id", "node1");
  const std::string data_dir = arg_value(args, "--data-dir", "data/" + node_id);
  const bool standalone = has_flag(args, "--standalone");

  std::cout << "raftkv-node " << raftkv::backend_version_string() << "\n";
  std::cout << "node_id=" << node_id << "\n";
  std::cout << "data_dir=" << data_dir << "\n";
  std::cout << "mode=" << (standalone ? "standalone" : "foreground") << "\n";
  std::cout << "status=ready\n";

  if (!standalone) {
    return 0;
  }

  std::signal(SIGINT, handle_signal);
  std::signal(SIGTERM, handle_signal);

  while (!shutdown_requested) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  std::cout << "status=stopped\n";
  return 0;
}
