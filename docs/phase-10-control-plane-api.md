# Phase 10: Control Plane API

Branch: `feat/control-plane-api`

## What This Phase Adds

This phase adds a real HTTP control-plane process:

- `GET /health`
- `GET /api/v1/cluster`
- `GET /api/v1/events`
- `POST /api/v1/commands`
- `POST /api/v1/faults/node`
- `POST /api/v1/demos/reset`

It also adds tests and run scripts:

```bash
./scripts/run_control_plane.sh
./scripts/test_control_plane.sh
```

## Why It Uses The Standard Library

The current backend has Raft logic and gRPC contracts, but it does not yet run a
networked Raft node server. This control plane therefore uses an in-memory
simulation that mirrors the behaviors already proven by backend tests.

That keeps the API shape useful for the frontend without pretending that live
node-to-node gRPC orchestration exists yet.

## Current Behavior

The control plane can:

- report a 3-node cluster snapshot
- accept `PUT`, `GET`, and `DELETE`
- simulate no-quorum behavior by marking nodes unavailable
- emit event history for leader election, log append, commit, apply, node stop,
  and quorum loss

## Next Step

The future gRPC node server can replace the simulated service internals. The HTTP
API can stay stable for the UI.

Next branch:

```text
feat/raft-observability-ui
```
