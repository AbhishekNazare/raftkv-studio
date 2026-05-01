# RaftKV Studio

RaftKV Studio is a phased distributed-systems project: a C++20 Raft-backed
key-value store, a small HTTP control plane, and a React/TypeScript dashboard
for observing consensus behavior.

The project is built branch-by-branch so each phase teaches one Raft or
engineering idea and leaves behind tests, docs, and a readable Git history.

## What Works Today

- Deterministic in-process 3-node Raft cluster.
- Leader election model and single-leader tracking.
- In-memory key-value state machine.
- Raft log append, lookup, conflict handling, and commit index advancement.
- Quorum replication: 3/3 and 2/3 writes commit, 1/3 writes do not.
- Follower catch-up after missed entries.
- Leader failover while preserving committed state.
- File-backed log, metadata, and KV state recovery primitives.
- Protobuf/gRPC message contracts and RPC mapper tests.
- Docker build and demo shell for backend nodes.
- Python standard-library control-plane API.
- React/TypeScript dashboard for cluster status, commands, events, faults, and
  snapshots.
- Snapshot creation, log compaction, and far-behind follower snapshot install in
  backend tests and guided demos.

## Architecture

```text
React UI
  |
  | HTTP
  v
Control Plane
  |
  | deterministic service simulation today
  | future gRPC clients
  v
C++ RaftKV backend
```

The C++ backend is the source of truth for Raft behavior. The control plane uses
a deterministic simulation so the dashboard and demo scripts can already tell
the project story before live multi-process Raft RPCs are wired end to end.

## Quick Start

Install prerequisites:

- CMake 3.22+
- C++20 compiler
- Protobuf and gRPC C++ development packages
- Python 3.10+
- Node.js 20+
- Docker, optional for container checks

Run the main checks:

```bash
./scripts/test_backend.sh
./scripts/test_control_plane.sh
./scripts/test_ui.sh
./scripts/demo_all.sh
```

Run the dashboard locally:

```bash
./scripts/run_control_plane.sh
./scripts/run_ui.sh
```

Open:

```text
http://127.0.0.1:5173
```

## Demo Scripts

```bash
./scripts/demo_leader_election.sh
./scripts/demo_log_replication.sh
./scripts/demo_follower_catchup.sh
./scripts/demo_leader_failover.sh
./scripts/demo_no_quorum.sh
./scripts/demo_network_partition.sh
./scripts/demo_snapshot_install.sh
./scripts/demo_control_plane.sh
./scripts/demo_all.sh
```

Each script prints what it proves and exits non-zero on failure.

## Repository Guide

- `backend/`: C++20 RaftKV core, storage primitives, protobuf/gRPC contracts,
  and unit tests.
- `control-plane/`: Python HTTP API used by demos and the UI.
- `ui/`: React/TypeScript observability dashboard.
- `scripts/`: local build, test, run, and demo entry points.
- `docs/`: phase notes, demo guide, architecture tradeoffs, and interview notes.

## Learning Path

The phase plan is in [docs/PHASE_PLAN.md](docs/PHASE_PLAN.md). The most useful
phase notes are:

- [Phase 5: Quorum Log Replication](docs/phase-5-quorum-log-replication.md)
- [Phase 6: Follower Catch-Up and Failover](docs/phase-6-follower-catchup-failover.md)
- [Phase 12: Fault Injection Demos](docs/phase-12-fault-injection-demos.md)
- [Phase 13: Snapshots and Log Compaction](docs/phase-13-snapshots-log-compaction.md)

## Honest Limits

This is an educational, portfolio-ready Raft project, not a production database.
The current dashboard talks to a simulated control plane, not live C++ nodes.
The backend has gRPC contracts and mapping tests, but the full multi-process
Raft server loop is intentionally left as future work. RocksDB integration,
membership changes, request deduplication, linearizable reads, and production
deployment hardening are also out of scope for the current milestone.

See [docs/INTERVIEW_GUIDE.md](docs/INTERVIEW_GUIDE.md) for how to explain those
tradeoffs clearly.
