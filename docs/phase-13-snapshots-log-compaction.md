# Phase 13: Snapshots And Log Compaction

Branch: `feat/snapshots-log-compaction`

Goal: stop treating the Raft log as an infinite list. A real Raft system keeps a
snapshot of committed state and compacts log entries already covered by that
snapshot.

## What Changed

- Added `Snapshot` and `SnapshotMetadata` to the C++ Raft core.
- Added state-machine restore support with `replace_all`.
- Added Raft log compaction while preserving the boundary term at the snapshot
  index.
- Added leader snapshot creation.
- Added far-behind follower snapshot install in the in-process cluster.
- Added a control-plane snapshot route and guided `snapshot-install` demo.
- Added dashboard snapshot status and a Create Snapshot action.

## Why This Matters

Before this phase, every committed command stayed in the log forever. That is
useful while learning replication, but it is not how production Raft systems
survive long-running workloads.

Snapshots give the node a compact replacement for old committed log entries:

```text
committed entries -> state machine -> snapshot
snapshot index    -> all log entries up to this point can be compacted
```

The important detail is the snapshot boundary. Raft still needs the term at the
last included index so future `AppendEntries` consistency checks can work even
after older entries are removed.

## Backend Behavior

`RaftNode::create_snapshot()` captures the committed KV state, records the last
included index and term, then asks `RaftLog` to compact entries covered by that
snapshot.

`InProcessCluster::sync_follower()` now detects when a follower is too far
behind for normal log catch-up. If the leader's compacted snapshot covers the
follower's next required index, the follower installs the snapshot before
continuing.

## Demo Behavior

The control plane exposes:

```text
POST /api/v1/snapshots/create
POST /api/v1/demos/run {"scenario":"snapshot-install"}
```

The dashboard shows:

- snapshot index
- compacted log start index
- a Create Snapshot button
- a Snapshot Install guided demo

The control plane remains a deterministic simulation. The C++ tests are the
source of truth for real log compaction and follower snapshot install behavior.

## Verification

```bash
./scripts/test_backend.sh
./scripts/test_control_plane.sh
./scripts/test_ui.sh
./scripts/demo_snapshot_install.sh
./scripts/demo_all.sh
```
