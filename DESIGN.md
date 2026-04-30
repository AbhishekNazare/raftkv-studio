# RaftKV Studio Design

This document tracks the architecture as the project evolves. The initial design
comes from the project brief in `RaftKV_Studio_README_v2.md`.

## High-Level Architecture

```text
React UI
  |
  | HTTP / WebSocket
  v
Control Plane Gateway
  |
  | gRPC
  v
3-node RaftKV cluster
```

## Backend Responsibilities

Each RaftKV node will own:

- Raft role state: follower, candidate, leader.
- Persistent term and vote metadata.
- Raft log entries.
- KV state machine.
- Peer replication state: `nextIndex` and `matchIndex`.
- Apply worker for committed entries.
- Snapshot and compaction state.
- Debug/event stream for observability.

## Correctness Rules

- A write is committed only after a majority acknowledges it.
- Followers acknowledge log entries only after durable persistence.
- Nodes reject stale-term RPCs.
- Leaders step down when they observe a higher term.
- A leader cannot commit entries without quorum.
- Applied KV state is derived from committed log entries.
- Logs are compacted only after a durable snapshot exists.

## Implementation Order

The backend starts in memory before networking or RocksDB. This keeps early Raft
logic testable and deterministic.

1. In-memory KV state machine.
2. In-memory Raft log.
3. Single-process Raft cluster simulation.
4. Quorum commit and follower catch-up.
5. Persistence.
6. gRPC multi-process cluster.
7. Docker demos.
8. Control plane and UI.

