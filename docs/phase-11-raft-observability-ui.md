# Phase 11: Raft Observability UI

Branch: `feat/raft-observability-ui`

## What This Phase Adds

This phase adds the first React/TypeScript dashboard:

- cluster topology
- node details
- command terminal
- fault controls
- event timeline
- KV state viewer
- API online/offline status

The UI talks to the Phase 10 control-plane API.

## Run

Start the control plane:

```bash
./scripts/run_control_plane.sh
```

Start the UI:

```bash
./scripts/run_ui.sh
```

Open:

```text
http://127.0.0.1:5173
```

## Test

```bash
./scripts/test_ui.sh
```

## Current Limits

The control plane still uses a simulated cluster service. The UI is already
wired to HTTP endpoints, so it can keep the same API when the backend switches
from simulation to real gRPC node clients.

## Next Phase

Next branch:

```text
feat/fault-injection-demos
```
