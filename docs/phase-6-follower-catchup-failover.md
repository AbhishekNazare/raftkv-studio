# Phase 6: Follower Catch-Up and Failover

Branch: `feat/follower-catchup-failover`

## What This Phase Adds

This phase demonstrates two important Raft behaviors:

- A follower can miss committed entries and later catch up.
- A cluster can elect a new leader after the old leader becomes unavailable.

The in-process cluster now supports:

- Marking nodes available or unavailable.
- Syncing a lagging follower from the current leader.
- Electing a new leader from the available majority side.
- Continuing writes after failover.

## Follower Catch-Up

In a 3-node cluster, a write can commit with only two nodes:

```text
node1 leader: append entry
node2 follower: append entry
node3 offline: misses entry

majority = 2/3
entry commits
```

When `node3` returns, the leader sends the missing entries. The follower appends
them, receives the leader's commit index, and applies the committed commands to
its state machine.

## Leader Failover

If the leader becomes unavailable, the remaining majority can elect a new leader.
Committed data remains available because the new leader already has the committed
log entries.

```text
node1 leader commits session:1
node1 becomes unavailable
node2 receives majority vote from node3
node2 becomes leader
cluster commits session:2
```

## Split-Brain Safety Preview

The isolated old leader may still believe it is leader locally. That is expected
in a partitioned distributed system. The important rule is that it cannot commit
new writes because it cannot reach majority.

This phase does not fully simulate bidirectional network partitions yet, but the
tests already show that the majority-side leader is the only one committing new
entries through the cluster.

## Commands

```bash
./scripts/test_backend.sh
```

Expected result:

```text
100% tests passed, 0 tests failed out of 6
```

## Next Phase

Next branch:

```text
feat/persistent-raft-storage
```

That phase introduces durable storage interfaces for term, vote, log entries,
and KV state so committed data can survive node restarts.
