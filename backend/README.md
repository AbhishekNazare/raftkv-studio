# RaftKV Backend

This directory contains the C++20 backend for RaftKV Studio.

Phase 1 only establishes the backend skeleton:

- CMake project definition.
- A small `raftkv_core` library.
- A minimal `raftkv-node` executable.
- A smoke test executable wired into CTest.

The first backend milestone intentionally avoids Raft logic. The goal is to make
the project buildable and testable before adding distributed systems behavior.

## Build

```bash
./scripts/build_backend.sh
```

## Test

```bash
./scripts/test_backend.sh
```

## Why Start This Small?

Raft has enough complexity on its own. A clean build/test skeleton gives every
future phase a stable place to add code and tests:

- Phase 2 adds the deterministic KV state machine.
- Phase 3 adds the Raft log model.
- Phase 4 adds an in-process Raft cluster.

