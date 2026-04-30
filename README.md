# RaftKV Studio

RaftKV Studio is a production-style distributed key-value store project built
around the Raft consensus algorithm. The goal is to build the system in phases
so each branch teaches and demonstrates one important distributed systems idea.

The final project will include:

- A C++20 Raft-backed key-value store.
- A `kvctl` CLI for commands and cluster inspection.
- A control-plane API for status, commands, demos, faults, and events.
- A React/TypeScript observability dashboard.
- Docker-based demo scripts for leader election, log replication, follower
  catch-up, leader failover, no quorum, split-brain prevention, crash recovery,
  snapshots, and log compaction.

## Current Status

Phase 0 is the repository foundation. The implementation has not started yet.
The current branch is intended to establish the roadmap, docs, and project
layout before backend code is added.

See [docs/PHASE_PLAN.md](docs/PHASE_PLAN.md) for the feature-by-feature build
plan, branch names, learning goals, and commit checkpoints.

## Core System Story

RaftKV Studio will run a 3-node cluster. One node becomes leader, accepts client
commands, appends them to a Raft log, replicates them to followers, commits them
after majority acknowledgment, and applies committed entries to a key-value
state machine.

The project is intentionally demo-driven. Every major feature should have tests
or scripts that prove the behavior instead of relying on explanation alone.

## Planned Stack

Backend:

- C++20
- CMake
- GoogleTest
- gRPC
- Protobuf
- RocksDB
- Docker

Control plane:

- FastAPI or Node.js
- HTTP
- WebSocket
- gRPC clients to Raft nodes

Frontend:

- React
- TypeScript
- Tailwind CSS
- React Flow
- Recharts
- WebSocket event stream

## Development Style

This repository should be built like a serious software engineering project:

- One branch per phase.
- Small commits with clear messages.
- Tests added alongside features.
- Docs updated when concepts become real.
- Demo scripts that explain what they prove.

## Current Commands

Run backend tests locally:

```bash
./scripts/test_backend.sh
```

Run current demo checks:

```bash
./scripts/demo_all.sh
```

Run control-plane tests:

```bash
./scripts/test_control_plane.sh
```

Run the control-plane API:

```bash
./scripts/run_control_plane.sh
```

Build and test inside Docker:

```bash
./scripts/docker_test_backend.sh
```

Start the current 3-node Docker shell:

```bash
./scripts/run_cluster.sh
```

The Docker nodes currently run in standalone mode. Live cross-container Raft
traffic will be wired after the gRPC node server is implemented.

## First Branches

```text
chore/project-foundation
feat/backend-cpp-skeleton
feat/in-memory-kv-state-machine
feat/raft-log-core
feat/in-process-raft-cluster
feat/quorum-log-replication
feat/follower-catchup-failover
feat/persistent-raft-storage
feat/grpc-raft-rpc
feat/docker-demo-cluster
feat/control-plane-api
feat/raft-observability-ui
feat/fault-injection-demos
feat/snapshots-log-compaction
chore/portfolio-polish
```
