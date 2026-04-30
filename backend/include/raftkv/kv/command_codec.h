#pragma once

#include <string>

#include "raftkv/common/result.h"
#include "raftkv/kv/command.h"

namespace raftkv::kv {

std::string encode_command(const Command& command);
Result<Command> decode_command(const std::string& encoded);

}  // namespace raftkv::kv

