# Phase 8: gRPC Raft RPC Contracts

Branch: `feat/grpc-raft-rpc`

## What This Phase Adds

This phase introduces the network contracts for RaftKV:

- `raft.proto` with `RequestVote` and `AppendEntries`.
- `kv.proto` with `Put`, `Get`, and `Delete`.
- `debug.proto` with node status inspection.
- `event.proto` with event streaming.
- CMake generation of C++ Protobuf and gRPC sources for `raft.proto`.
- A mapper layer between internal C++ Raft structs and generated Protobuf
  message types.

## Why This Is a Separate Phase

The previous phases used in-process method calls. That was deliberate: it let us
prove Raft behavior without network complexity.

This phase starts the transition from:

```text
C++ method call
```

to:

```text
gRPC request/response
```

The consensus data is the same. Only the transport changes.

## Contracts Added

Raft:

```text
RequestVote(term, candidate_id, last_log_index, last_log_term)
AppendEntries(term, leader_id, prev_log_index, prev_log_term, entries, leader_commit)
```

KV:

```text
Put(key, value)
Get(key)
Delete(key)
```

Debug:

```text
NodeStatus()
```

Events:

```text
StreamEvents(node_id)
```

## Current Limits

This phase does not start real gRPC servers yet. It establishes generated
contracts and proves that internal Raft request/response structs can round-trip
through Protobuf messages.

The next networking step can implement `RaftService` by delegating generated
gRPC calls into `RaftNode`.

## Commands

```bash
./scripts/test_backend.sh
```

Expected result:

```text
100% tests passed, 0 tests failed out of 8
```

## Local Dependencies

This phase uses:

```text
protobuf
grpc
pkg-config
```

Installed locally with Homebrew during this phase.
