# RaftKV Control Plane

The control plane exposes HTTP endpoints that the future UI can use for cluster
inspection, command submission, and event history.

This phase intentionally uses only Python's standard library. The backend does
not yet expose a live gRPC node server, so the control plane currently owns a
deterministic in-memory simulation that mirrors the backend behaviors already
covered by C++ tests.

## Run

```bash
./scripts/run_control_plane.sh
```

Default URL:

```text
http://127.0.0.1:8090
```

## Test

```bash
./scripts/test_control_plane.sh
```

## Endpoints

```text
GET  /health
GET  /api/v1/cluster
GET  /api/v1/events
POST /api/v1/commands
POST /api/v1/faults/node
POST /api/v1/faults/partition
POST /api/v1/faults/heal
POST /api/v1/demos/reset
POST /api/v1/demos/run
```

## Current Limits

The API is real HTTP, but its cluster data is simulated. Once the backend node
server exists, the service layer can replace the simulation with gRPC clients.
