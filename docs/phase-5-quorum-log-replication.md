# Phase 5: Quorum Log Replication

Branch: `feat/quorum-log-replication`

## What This Phase Adds

This phase connects the earlier pieces into the first end-to-end write path:

- A leader appends an encoded command to its local Raft log.
- The leader sends `AppendEntries` to followers in an in-process cluster.
- Followers append the entry only after `prevLogIndex` and `prevLogTerm` match.
- A write commits only after majority acknowledgement.
- Committed entries are applied to each available node's KV state machine.
- A write without quorum remains uncommitted and unapplied.

## Why This Matters

This is the first phase where Raft's core safety rule becomes visible:

```text
append locally does not mean committed
replicated to majority means committed
committed means safe to apply
```

The leader may have an entry in its local log while isolated, but the state
machine does not expose that value until quorum is reached.

## Write Flow Implemented

```text
client command
  -> leader appends log entry
  -> leader sends AppendEntries to followers
  -> majority ACKs
  -> leader advances commitIndex
  -> leader applies command
  -> leader broadcasts commitIndex
  -> followers apply command
```

## Current Limits

This phase keeps replication intentionally simple:

- Replication is still in-process.
- Followers that are offline do not catch up yet.
- The leader does not backtrack `nextIndex` yet.
- There is no real network, disk, or gRPC.

Follower catch-up and failover come next.

## Commands

```bash
./scripts/test_backend.sh
```

Expected result:

```text
100% tests passed, 0 tests failed out of 5
```

## Next Phase

Next branch:

```text
feat/follower-catchup-failover
```

That phase will bring offline followers back, replicate missing entries, and
prove that a new leader can continue after leader failure.
