# Phase 1: Backend C++ Skeleton

Branch: `feat/backend-cpp-skeleton`

## What This Phase Adds

This phase creates the first buildable backend slice:

- `backend/CMakeLists.txt` defines the C++20 project.
- `raftkv_core` is the first backend library.
- `raftkv-node` is the first executable.
- `raftkv_smoke_test` is the first test binary.
- `scripts/build_backend.sh` and `scripts/test_backend.sh` give repeatable
  commands for local development and future CI.

## Why We Start Here

Raft is complicated enough without also debugging project structure. This phase
keeps behavior intentionally small so the build system, source layout, and test
path are proven before consensus code appears.

The important engineering habit is:

```text
every future feature enters through the same build and test path
```

That keeps later branches easy to review.

## Current Backend Shape

```text
backend/
├── CMakeLists.txt
├── include/raftkv/common/
│   ├── result.h
│   ├── status.h
│   └── types.h
├── src/
│   ├── common/version.cpp
│   └── main.cpp
└── tests/unit/smoke_test.cpp
```

## Concepts Introduced

`NodeId`, `Term`, and `LogIndex` are domain types. They make Raft code easier to
read later because terms and log indexes are not just anonymous integers.

`Status` gives functions a standard way to report failure without throwing
exceptions everywhere.

`Result<T>` gives functions a way to return either a value or a `Status`.

The smoke test verifies that the core library links correctly and that the first
shared types are usable from a test binary.

## Commands

```bash
./scripts/build_backend.sh
./scripts/test_backend.sh
```

Expected test result:

```text
100% tests passed, 0 tests failed out of 1
```

## Next Phase

Next branch:

```text
feat/in-memory-kv-state-machine
```

That phase adds deterministic `PUT`, `GET`, and `DELETE` command handling before
we add Raft replication.

