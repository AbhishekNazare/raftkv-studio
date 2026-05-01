# Demo Guide

This guide is for quickly showing RaftKV Studio to a reviewer.

For the full command-by-command walkthrough with failure cases, use
[PROJECT_DEMONSTRATION_RUNBOOK.md](PROJECT_DEMONSTRATION_RUNBOOK.md).

## 1. Run The Checks

```bash
./scripts/test_backend.sh
./scripts/test_control_plane.sh
./scripts/test_ui.sh
./scripts/demo_all.sh
```

Expected result: every command exits successfully.

## 2. Start The Dashboard

Recommended single-command start:

```bash
./scripts/run_dev.sh
```

This clears stale RaftKV dev ports, starts the control plane on `8090`, and
starts the UI on `5173`.

If you need to stop stale dev servers manually:

```bash
./scripts/stop_dev.sh
```

Separate-terminal start:

Terminal 1:

```bash
./scripts/run_control_plane.sh
```

Terminal 2:

```bash
./scripts/run_ui.sh
```

Open:

```text
http://127.0.0.1:5173
```

## 3. Show A Normal Write

Use the Command Terminal:

```text
PUT user:1 Abhishek
```

Expected result:

- the command reports `committed true`
- the KV State panel shows `user:1`
- the Event Timeline shows append, commit, and apply events

## 4. Show No Quorum

Click:

```text
No Quorum
```

Expected result:

- the demo reports passed
- the timeline includes `QUORUM_LOST`
- the rejected write does not appear in KV State

## 5. Show Failover

Click:

```text
Leader Failover
```

Expected result:

- the old leader is stopped
- a new leader is elected
- both committed session keys remain visible

## 6. Show Network Partition Safety

Click:

```text
Network Partition
```

Expected result:

- the isolated old leader loses authority
- the majority side elects a new leader
- the old leader steps down after healing

## 7. Show Snapshot Install

Click:

```text
Snapshot Install
```

Expected result:

- snapshot index advances to `2`
- compacted log start advances to `3`
- the lagging follower installs the snapshot

## Capture Notes

For portfolio screenshots, capture these dashboard states:

- fresh cluster with node1 as leader
- committed command visible in KV State
- network partition demo after it passes
- snapshot install demo with snapshot metrics visible

Keep captions factual. The current UI talks to the Python control-plane
simulation; backend Raft behavior is proven by C++ tests.
