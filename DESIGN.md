# RaftKV Studio Design

This document describes the current architecture and the tradeoffs behind it.

## System Shape

```text
Browser dashboard
  |
  | HTTP JSON
  v
Python control plane
  |
  | planned gRPC client boundary
  v
C++ RaftKV nodes
```

The backend owns the consensus model. The control plane and UI make that model
observable and easy to demonstrate.

## Backend Modules

- `kv`: command encoding and deterministic key-value state-machine behavior.
- `raft`: log entries, log matching, node role state, in-process cluster tests,
  quorum replication, failover, snapshots, and compaction.
- `storage`: file-backed log, metadata, and KV store primitives.
- `net`: protobuf/gRPC contracts and mapper tests.

The in-process cluster exists to make correctness behavior deterministic before
adding timing, sockets, retries, and process supervision.

## Control Plane

The control plane is a Python standard-library HTTP service. It exposes:

- cluster status
- command submission
- event history
- node fault controls
- guided demo scenarios
- snapshot creation

Today it uses a deterministic simulation aligned with backend tests. That is a
deliberate boundary: it lets the UI, demos, and docs mature without pretending
that the live C++ cluster server is complete.

## Dashboard

The React dashboard is an operator-style view:

- cluster topology
- selected-node metrics
- command terminal
- fault controls
- guided demos
- event timeline
- KV state
- snapshot status

It avoids marketing-page layout and focuses on repeated inspection workflows.

## Correctness Rules

- A write commits only after a majority acknowledges it.
- A node applies only committed log entries.
- A leader with no quorum cannot commit.
- A restarted or lagging follower must catch up before it can serve the same
  committed state.
- A far-behind follower may install a snapshot instead of receiving compacted
  log entries.
- Log compaction preserves the snapshot boundary term for future log-matching
  checks.

## Important Tradeoffs

- The project favors deterministic tests before distributed runtime complexity.
- The control plane currently simulates cluster behavior instead of driving live
  C++ nodes.
- gRPC contracts exist, but the full multi-process Raft server loop remains
  future work.
- Storage is file-backed and testable, but not tuned for throughput or crash
  consistency the way a production database would be.
- Snapshots are in-memory state snapshots today; durable snapshot files are a
  natural next step.

## Future Work

- Live C++ gRPC node server and client wiring.
- Durable snapshot files and install-snapshot RPC.
- Linearizable reads and request deduplication.
- Better failure injection with delayed, dropped, duplicate, and reordered RPCs.
- Benchmarks for write latency, catch-up time, and compaction impact.
- Optional RocksDB storage engine.
