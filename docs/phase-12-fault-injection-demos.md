# Phase 12: Fault Injection Demos

Branch: `feat/fault-injection-demos`

## What This Phase Adds

This phase adds named fault/demo scenarios to the control plane and UI:

- no quorum
- leader failover
- network partition

The control-plane API now supports:

```text
POST /api/v1/faults/partition
POST /api/v1/faults/heal
POST /api/v1/demos/run
```

## Why This Matters

The project already had generic node availability controls. Named demos make the
distributed-systems story easier to show because each scenario returns a clear
pass/fail result and emits the events that explain what happened.

## UI Changes

The dashboard now has a guided demos panel with buttons for:

- No Quorum
- Leader Failover
- Network Partition

## Scripts

```bash
./scripts/demo_network_partition.sh
./scripts/demo_control_plane.sh
./scripts/demo_all.sh
```

## Current Limits

The fault scenarios are still control-plane simulations. They are intentionally
aligned with backend unit tests and will later be wired to real node controls
once the backend exposes live server processes.

## Next Phase

Next branch:

```text
feat/snapshots-log-compaction
```
