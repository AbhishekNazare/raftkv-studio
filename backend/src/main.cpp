#include <iostream>

#include "raftkv/common/types.h"

int main() {
  std::cout << "raftkv-node " << raftkv::backend_version_string() << "\n";
  std::cout << "backend skeleton ready\n";
  return 0;
}

