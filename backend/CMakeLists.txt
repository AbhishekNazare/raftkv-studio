cmake_minimum_required(VERSION 3.22)

project(
  raftkv_backend
  VERSION 0.1.0
  DESCRIPTION "C++20 backend for RaftKV Studio"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(RAFTKV_BUILD_TESTS "Build backend tests" ON)

add_library(raftkv_core
  src/common/version.cpp
  src/kv/command.cpp
  src/kv/command_codec.cpp
  src/kv/state_machine.cpp
)

target_include_directories(raftkv_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(raftkv_core PUBLIC cxx_std_20)

add_executable(raftkv-node
  src/main.cpp
)

target_link_libraries(raftkv-node
  PRIVATE
    raftkv_core
)

if(RAFTKV_BUILD_TESTS)
  enable_testing()

  add_executable(raftkv_smoke_test
    tests/unit/smoke_test.cpp
  )

  target_link_libraries(raftkv_smoke_test
    PRIVATE
      raftkv_core
  )

  add_test(NAME raftkv_smoke_test COMMAND raftkv_smoke_test)

  add_executable(raftkv_state_machine_test
    tests/unit/state_machine_test.cpp
  )

  target_link_libraries(raftkv_state_machine_test
    PRIVATE
      raftkv_core
  )

  add_test(NAME raftkv_state_machine_test COMMAND raftkv_state_machine_test)
endif()
