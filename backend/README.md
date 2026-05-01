# RaftKV Backend

This directory contains the C++20 RaftKV core.

## Current Scope

- KV command model and deterministic state machine.
- Raft log append, lookup, conflict replacement, commit advancement, and
  compaction.
- In-process cluster for deterministic leader, replication, catch-up, and
  failover tests.
- File-backed metadata, log, and KV storage primitives.
- Protobuf/gRPC contracts and mapper tests.
- Snapshot creation and snapshot install for far-behind followers.

## Build

```bash
./scripts/build_backend.sh
```

## Test

```bash
./scripts/test_backend.sh
```

## Design Note

The backend deliberately starts with deterministic in-process tests before the
full multi-process server loop. That keeps Raft correctness work easy to reason
about and gives later networking work a stable behavioral contract.
