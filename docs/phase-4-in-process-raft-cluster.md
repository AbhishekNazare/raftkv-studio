# Phase 4: In-Process Raft Cluster

Branch: `feat/in-process-raft-cluster`

## What This Phase Adds

This phase introduces the first `RaftNode` behavior:

- Node roles: follower, candidate, leader.
- Term tracking.
- One vote per node per term.
- Self-vote when starting an election.
- RequestVote handling with log freshness checks.
- AppendEntries heartbeat handling.
- Step down when a node sees a higher term.
- A deterministic 3-node election test.

## Why This Is Still In-Process

The Raft rules are easier to test without networking, timers, threads, or disk.
This phase uses direct C++ method calls that represent the future RPCs:

```text
RequestVote
AppendEntries
```

Later, gRPC will carry the same request and response data between processes.
The consensus rules should not depend on the transport.

## Election Rule

When a follower has not heard from a leader, it can start an election:

```text
Follower -> Candidate
term += 1
vote for self
send RequestVote to peers
become leader after majority votes
```

The current test drives this deterministically by manually starting an election
on one node and sending vote requests to the other two nodes.

## Vote Safety

A follower grants at most one vote per term. It also checks that the candidate's
log is at least as fresh as its own log:

```text
higher lastLogTerm wins
if terms tie, higher/equal lastLogIndex wins
```

This is how Raft prevents a candidate with stale history from becoming leader.

## Heartbeat Handling

Heartbeats are AppendEntries requests with no new entries. A node rejects stale
leaders, accepts current/newer leaders if the previous log reference matches,
and steps down when it sees a higher term.

## Current Limits

This phase does not include:

- Real election timers.
- Heartbeat loops.
- Log entry replication through `AppendEntries`.
- Quorum commit calculation.
- Applying committed entries to the KV state machine.

Those come in the next phases.

## Commands

```bash
./scripts/test_backend.sh
```

Expected result:

```text
100% tests passed, 0 tests failed out of 4
```

## Next Phase

Next branch:

```text
feat/quorum-log-replication
```

That phase makes the leader replicate client commands to followers, track
`nextIndex` and `matchIndex`, commit after majority replication, and apply
committed entries to the KV state machine.

