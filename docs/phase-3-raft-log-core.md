# Phase 3: Raft Log Core

Branch: `feat/raft-log-core`

## What This Phase Adds

This phase introduces the in-memory Raft log:

- `LogEntry` stores an index, term, and encoded command.
- `RaftLog::append` appends local entries with monotonically increasing indexes.
- `RaftLog::append_from_leader` implements the follower-side log matching rule.
- Conflicting uncommitted suffixes are replaced by the leader's entries.
- Committed entries cannot be replaced.
- `commitIndex` can advance, but never move backward or past the end of the log.

## Why This Matters

Raft safety depends on every node agreeing about log history before accepting new
entries. A leader sends:

```text
prevLogIndex
prevLogTerm
entries[]
```

A follower accepts the append only if its local log contains the previous entry
with the same term. If that check fails, the follower rejects the append and the
leader must retry from an earlier index.

This rule prevents divergent histories from silently continuing.

## Conflict Replacement

If a follower has an uncommitted suffix that conflicts with the leader, the
leader wins:

```text
follower before: 1/t1  2/t2 stale  3/t2 stale
leader sends:              2/t3 fresh  3/t3 fresh
follower after:  1/t1  2/t3 fresh  3/t3 fresh
```

This is safe only for uncommitted entries. Once an entry is committed, this
implementation rejects attempts to replace it.

## Current Limits

This phase does not elect leaders or calculate quorum. It only models the local
log behavior that later phases will use from a `RaftNode`.

## Commands

```bash
./scripts/test_backend.sh
```

Expected result:

```text
100% tests passed, 0 tests failed out of 3
```

## Next Phase

Next branch:

```text
feat/in-process-raft-cluster
```

That phase introduces `RaftNode`, roles, terms, RequestVote handling,
AppendEntries heartbeats, and deterministic single-process cluster tests.

