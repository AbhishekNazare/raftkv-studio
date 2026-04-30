# Phase 2: In-Memory KV State Machine

Branch: `feat/in-memory-kv-state-machine`

## What This Phase Adds

This phase adds deterministic key-value command handling before Raft exists:

- `Command` models mutations that Raft will replicate later.
- `CommandCodec` serializes commands into a stable string form.
- `StateMachine` applies committed commands to in-memory key-value state.
- Unit tests cover `PUT`, `GET`, `DELETE`, overwrites, invalid commands, and
  command codec round trips.

## Why This Comes Before Raft

Raft does not replicate arbitrary C++ method calls. It replicates log entries.
For this project, a log entry will eventually contain an encoded command:

```text
PUT user:1 Abhishek
DELETE user:1
```

Once a log entry becomes committed, the node applies that command to its local
state machine. If every node applies the same committed commands in the same
order, every node reaches the same key-value state.

## Important Design Rule

The state machine must be deterministic.

Given the same ordered command stream:

```text
PUT city Gainesville
PUT city Seattle
DELETE city
```

every node must end with the same state:

```text
city is not found
```

That is why the state machine does not know about leaders, followers, elections,
or quorum. It only knows how to apply already-committed commands.

## Current Behavior

`PUT key value`

- Adds the key if missing.
- Overwrites the key if present.

`GET key`

- Returns the value if present.
- Returns `kNotFound` if missing.

`DELETE key`

- Removes the key if present.
- Does nothing if missing.

Invalid command:

- Empty keys are rejected.
- Invalid commands do not mutate state.

## Commands

```bash
./scripts/test_backend.sh
```

Expected result:

```text
100% tests passed, 0 tests failed out of 2
```

## Next Phase

Next branch:

```text
feat/raft-log-core
```

That phase introduces Raft log entries, append behavior, lookup by index,
previous-index validation, and conflict replacement.

