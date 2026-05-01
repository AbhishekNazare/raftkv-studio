# Phase 15: Modern Interactive UI

Branch: `feat/modern-interactive-ui`

Goal: replace the first dashboard with a more polished, interactive control
room for demonstrating Raft behavior.

## What Changed

- Rebuilt the dashboard layout around a live cluster map.
- Added animated replication links and selectable node controls.
- Added explicit Start/Stop, Isolate, and Heal actions for the selected node.
- Added better error handling so failed API responses are shown as real action
  failures instead of corrupting UI state.
- Added richer guided demo cards for no quorum, failover, partition safety, and
  snapshot install.
- Added snapshot progress, event timeline, KV state, quorum status, and command
  feedback in a denser operator-style layout.

## Why This Matters

The backend tests prove the Raft behavior, but the UI is what makes the project
easy to demonstrate. This phase turns the frontend from a basic status page into
a proper interactive demo surface.

## Verification

```bash
./scripts/test_ui.sh
./scripts/test_control_plane.sh
```

Manual check:

```bash
./scripts/run_control_plane.sh
./scripts/run_ui.sh
```

Then open the Vite URL and run:

- a normal `PUT`
- `No Quorum`
- `Leader Failover`
- `Partition Safety`
- `Snapshot Install`
- selected-node `Isolate` and `Heal`
