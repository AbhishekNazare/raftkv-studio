# RaftKV Studio Testing Strategy

Testing is part of the project story. Each major feature should prove one
distributed systems property.

## Test Layers

Unit tests:

- Command encoding and decoding.
- KV state machine behavior.
- Raft log append, lookup, and conflict replacement.
- Election timeout behavior.
- Snapshot manager behavior.

Integration tests:

- Exactly one leader is elected.
- Writes are committed after majority replication.
- Followers catch up after missing entries.
- Leader failover preserves committed data.
- No quorum prevents writes.
- Crash recovery restores committed state.
- Snapshot install recovers a far-behind follower.

Fault tests:

- Delayed messages.
- Dropped messages.
- Duplicate messages.
- Reordered messages.
- Stale terms.
- Network partitions.

## Demo Scripts

Demo scripts should be runnable documentation. Each script should print:

- What scenario is being demonstrated.
- What behavior is expected.
- What actually happened.
- Whether the result passed.

Planned scripts:

```text
scripts/demo_leader_election.sh
scripts/demo_log_replication.sh
scripts/demo_follower_catchup.sh
scripts/demo_leader_failover.sh
scripts/demo_no_quorum.sh
scripts/demo_network_partition.sh
scripts/demo_crash_recovery.sh
scripts/demo_snapshot_recovery.sh
scripts/demo_snapshot_install.sh
scripts/demo_all.sh
```

## Minimum Resume-Strength Scenarios

- Exactly one leader is elected.
- `PUT` commits with majority.
- A follower missing entries catches up.
- Leader crash triggers a new election.
- No quorum rejects writes.
- Old isolated leader cannot commit after partition.
- Node restart preserves committed data.
- Duplicate `AppendEntries` does not duplicate logs.
- Reordered `AppendEntries` is rejected.
- Snapshot compacts old logs.
- Lagging follower installs a snapshot.

