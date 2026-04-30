# Phase 7: Persistent Raft Storage

Branch: `feat/persistent-raft-storage`

## What This Phase Adds

This phase introduces durable storage interfaces:

- `MetadataStore` for `currentTerm` and `votedFor`.
- `LogStore` for Raft log entries.
- `KVStore` for KV state snapshots.

It also adds dependency-free file-backed implementations:

- `FileMetadataStore`
- `FileLogStore`
- `FileKVStore`

## Why Interfaces First

Raft has strict persistence rules. A node must be able to recover its term, vote,
log, and committed state after restart. But the consensus code should not care
whether that data lives in plain files, RocksDB, or another storage engine.

This phase uses simple files so the persistence contract is easy to test. Later,
RocksDB can replace these implementations behind the same interfaces.

## Persistence Rules Introduced

Metadata:

```text
currentTerm
votedFor
```

These values prevent a restarted node from voting twice in the same term or
forgetting that it has already observed a higher term.

Log:

```text
index term encodedCommand
```

The log must survive process restart because committed and uncommitted entries
both affect future Raft decisions.

KV snapshot:

```text
encoded PUT commands for current key/value state
```

This is a first simple version of durable state-machine recovery. Later snapshot
metadata and log compaction will build on this.

## Current Limits

This phase does not wire persistence into `RaftNode` yet. It proves the storage
contracts and file-backed implementations independently first. The next backend
integration step can make nodes load and flush through these interfaces.

## Commands

```bash
./scripts/test_backend.sh
```

Expected result:

```text
100% tests passed, 0 tests failed out of 7
```

## Next Phase

Next branch:

```text
feat/grpc-raft-rpc
```

That phase starts turning the in-process request/response objects into real
network-facing Protobuf and gRPC contracts.
