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

find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

set(RAFTKV_PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/proto)
set(RAFTKV_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${RAFTKV_GENERATED_DIR})

set(RAFTKV_RAFT_PROTO ${RAFTKV_PROTO_DIR}/raft.proto)
set(RAFTKV_RAFT_PROTO_SRCS
  ${RAFTKV_GENERATED_DIR}/raft.pb.cc
  ${RAFTKV_GENERATED_DIR}/raft.grpc.pb.cc
)
set(RAFTKV_RAFT_PROTO_HDRS
  ${RAFTKV_GENERATED_DIR}/raft.pb.h
  ${RAFTKV_GENERATED_DIR}/raft.grpc.pb.h
)

add_custom_command(
  OUTPUT
    ${RAFTKV_RAFT_PROTO_SRCS}
    ${RAFTKV_RAFT_PROTO_HDRS}
  COMMAND $<TARGET_FILE:protobuf::protoc>
  ARGS
    --proto_path=${RAFTKV_PROTO_DIR}
    --cpp_out=${RAFTKV_GENERATED_DIR}
    --grpc_out=${RAFTKV_GENERATED_DIR}
    --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
    ${RAFTKV_RAFT_PROTO}
  DEPENDS ${RAFTKV_RAFT_PROTO}
  COMMENT "Generating Raft protobuf and gRPC sources"
)

add_library(raftkv_proto
  ${RAFTKV_RAFT_PROTO_SRCS}
)

target_include_directories(raftkv_proto
  PUBLIC
    ${RAFTKV_GENERATED_DIR}
)

target_link_libraries(raftkv_proto
  PUBLIC
    protobuf::libprotobuf
    gRPC::grpc++
)

add_library(raftkv_core
  src/common/version.cpp
  src/kv/command.cpp
  src/kv/command_codec.cpp
  src/kv/state_machine.cpp
  src/raft/in_process_cluster.cpp
  src/raft/raft_node.cpp
  src/raft/raft_log.cpp
  src/storage/file_kv_store.cpp
  src/storage/file_log_store.cpp
  src/storage/file_metadata_store.cpp
  src/net/raft_rpc_mapper.cpp
)

target_include_directories(raftkv_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(raftkv_core PUBLIC cxx_std_20)

target_link_libraries(raftkv_core
  PUBLIC
    raftkv_proto
)

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

  add_executable(raftkv_log_test
    tests/unit/raft_log_test.cpp
  )

  target_link_libraries(raftkv_log_test
    PRIVATE
      raftkv_core
  )

  add_test(NAME raftkv_log_test COMMAND raftkv_log_test)

  add_executable(raftkv_node_test
    tests/unit/raft_node_test.cpp
  )

  target_link_libraries(raftkv_node_test
    PRIVATE
      raftkv_core
  )

  add_test(NAME raftkv_node_test COMMAND raftkv_node_test)

  add_executable(raftkv_replication_test
    tests/unit/replication_test.cpp
  )

  target_link_libraries(raftkv_replication_test
    PRIVATE
      raftkv_core
  )

  add_test(NAME raftkv_replication_test COMMAND raftkv_replication_test)

  add_executable(raftkv_failover_test
    tests/unit/failover_test.cpp
  )

  target_link_libraries(raftkv_failover_test
    PRIVATE
      raftkv_core
  )

  add_test(NAME raftkv_failover_test COMMAND raftkv_failover_test)

  add_executable(raftkv_storage_test
    tests/unit/storage_test.cpp
  )

  target_link_libraries(raftkv_storage_test
    PRIVATE
      raftkv_core
  )

  add_test(NAME raftkv_storage_test COMMAND raftkv_storage_test)

  add_executable(raftkv_rpc_mapper_test
    tests/unit/rpc_mapper_test.cpp
  )

  target_link_libraries(raftkv_rpc_mapper_test
    PRIVATE
      raftkv_core
  )

  add_test(NAME raftkv_rpc_mapper_test COMMAND raftkv_rpc_mapper_test)
endif()
