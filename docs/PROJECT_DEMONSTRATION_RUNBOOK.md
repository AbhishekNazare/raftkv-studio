# Project Demonstration Runbook

Use this file when you want to demonstrate RaftKV Studio step by step. It
includes the exact commands to run, what to click in the UI, what failure cases
to show, and what each result proves.

## 0. What To Say First

Short explanation:

```text
RaftKV Studio is a phased distributed-systems project. The C++ backend proves
core Raft behavior with deterministic tests. The Python control plane and React
dashboard make those behaviors easy to inspect and demo.
```

Important honesty note:

```text
The dashboard currently talks to a deterministic Python control-plane
simulation. The C++ backend is where the Raft behavior is tested. A future
phase would wire the dashboard to live C++ nodes over gRPC.
```

## 1. Prerequisites

You should have:

- CMake 3.22+
- a C++20 compiler
- Protobuf and gRPC C++ development packages
- Python 3.10+
- Node.js 20+
- Docker, optional

From the repo root:

```bash
pwd
git status --short --branch
```

Expected:

```text
## main...origin/main
```

## 2. Fresh Verification Commands

Run these before any demo:

```bash
./scripts/test_backend.sh
./scripts/test_control_plane.sh
./scripts/test_ui.sh
./scripts/demo_all.sh
```

What this proves:

- backend builds and C++ tests pass
- control-plane API behavior is tested
- UI TypeScript and production build work
- all scripted demos pass

Optional Docker check:

```bash
./scripts/docker_test_backend.sh
```

Use this only if Docker is running.

## 3. Start The Project Locally

Recommended:

```bash
./scripts/run_dev.sh
```

Expected:

```text
Control plane: http://127.0.0.1:8090
UI: http://127.0.0.1:5173
```

Open:

```text
http://127.0.0.1:5173
```

Use this cleanup command if you previously started several dev servers and Vite
keeps jumping to `5174`, `5175`, or another port:

```bash
./scripts/stop_dev.sh
```

Separate-terminal mode:

Open two terminals.

Terminal 1:

```bash
./scripts/run_control_plane.sh
```

Expected:

```text
RaftKV control plane listening on http://127.0.0.1:8090
```

Terminal 2:

```bash
./scripts/run_ui.sh
```

Open:

```text
http://127.0.0.1:5173
```

Expected UI state:

- sidebar says `Control plane online`
- top bar says `node1 is leader`
- three nodes are visible
- majority is `2`

## 4. Quick API Smoke Test

You can show the HTTP API without the UI:

```bash
curl -s http://127.0.0.1:8090/health
curl -s http://127.0.0.1:8090/api/v1/cluster
curl -s http://127.0.0.1:8090/api/v1/events
```

Run a committed write:

```bash
curl -s -X POST http://127.0.0.1:8090/api/v1/commands \
  -H 'Content-Type: application/json' \
  -d '{"command":"PUT","key":"user:1","value":"Abhishek"}'
```

Expected:

```text
"committed": true
"acknowledgements": 3
```

Read it back:

```bash
curl -s -X POST http://127.0.0.1:8090/api/v1/commands \
  -H 'Content-Type: application/json' \
  -d '{"command":"GET","key":"user:1"}'
```

Expected:

```text
"value": "Abhishek"
```

## 5. UI Walkthrough: Normal Write

In the dashboard Command Terminal:

```text
PUT user:1 Abhishek
```

Click:

```text
Run
```

Expected:

- terminal output says `committed true`
- acknowledgements show `3/3`
- KV State shows `user:1`
- Event Timeline includes `LOG_APPENDED`, `ENTRY_COMMITTED`, and
  `ENTRY_APPLIED`

What to say:

```text
This is the normal Raft path: the leader appends the command, a majority
acknowledges it, the entry commits, then the state machine applies it.
```

## 6. Failure Case: No Quorum

UI path:

```text
Guided Demos -> No Quorum
```

Command-line path:

```bash
./scripts/demo_no_quorum.sh
```

Direct API path:

```bash
curl -s -X POST http://127.0.0.1:8090/api/v1/demos/run \
  -H 'Content-Type: application/json' \
  -d '{"scenario":"no-quorum"}'
```

Expected:

- demo reports passed
- write is not committed
- Event Timeline includes `QUORUM_LOST`
- rejected key does not appear in KV State

What this proves:

```text
A leader cannot safely commit a write with only one node available in a
three-node cluster. Majority is required.
```

## 7. Failure Case: Leader Failover

UI path:

```text
Guided Demos -> Leader Failover
```

Command-line path:

```bash
./scripts/demo_leader_failover.sh
```

Direct API path:

```bash
curl -s -X POST http://127.0.0.1:8090/api/v1/demos/run \
  -H 'Content-Type: application/json' \
  -d '{"scenario":"leader-failover"}'
```

Expected:

- old leader is stopped
- a new leader is elected
- committed data remains visible
- demo reports passed

What this proves:

```text
Committed state survives leader loss. The cluster can elect another available
node and continue committing writes.
```

## 8. Failure Case: Network Partition Safety

UI path:

```text
Guided Demos -> Network Partition
```

Command-line path:

```bash
./scripts/demo_network_partition.sh
```

Direct API path:

```bash
curl -s -X POST http://127.0.0.1:8090/api/v1/demos/run \
  -H 'Content-Type: application/json' \
  -d '{"scenario":"network-partition"}'
```

Expected:

- old leader is isolated
- majority side elects a new leader
- the old leader steps down after healing
- Event Timeline includes `NETWORK_PARTITIONED` and `STALE_TERM_REJECTED`

What this proves:

```text
The project demonstrates split-brain prevention at the demo level: the isolated
old leader cannot remain authoritative after the majority side moves on.
```

## 9. Snapshot And Log Compaction Demo

UI path:

```text
Guided Demos -> Snapshot Install
```

Command-line path:

```bash
./scripts/demo_snapshot_install.sh
```

Direct API path:

```bash
curl -s -X POST http://127.0.0.1:8090/api/v1/demos/run \
  -H 'Content-Type: application/json' \
  -d '{"scenario":"snapshot-install"}'
```

Expected:

- demo reports passed
- snapshot index becomes `2`
- compacted log start index becomes `3`
- Event Timeline includes `SNAPSHOT_CREATED`, `LOG_COMPACTED`, and
  `SNAPSHOT_INSTALLED`

What this proves:

```text
Old committed log entries can be represented by a snapshot. A far-behind
follower can install the snapshot instead of receiving log entries that the
leader has already compacted.
```

## 10. Manual Fault Controls

Reset first:

```bash
curl -s -X POST http://127.0.0.1:8090/api/v1/demos/reset \
  -H 'Content-Type: application/json' \
  -d '{}'
```

Stop two followers:

```bash
curl -s -X POST http://127.0.0.1:8090/api/v1/faults/node \
  -H 'Content-Type: application/json' \
  -d '{"nodeId":"node2","available":false}'

curl -s -X POST http://127.0.0.1:8090/api/v1/faults/node \
  -H 'Content-Type: application/json' \
  -d '{"nodeId":"node3","available":false}'
```

Try a write:

```bash
curl -s -X POST http://127.0.0.1:8090/api/v1/commands \
  -H 'Content-Type: application/json' \
  -d '{"command":"PUT","key":"payment:55","value":"success"}'
```

Expected:

```text
"committed": false
"acknowledgements": 1
"majority": 2
```

Bring nodes back:

```bash
curl -s -X POST http://127.0.0.1:8090/api/v1/faults/node \
  -H 'Content-Type: application/json' \
  -d '{"nodeId":"node2","available":true}'

curl -s -X POST http://127.0.0.1:8090/api/v1/faults/node \
  -H 'Content-Type: application/json' \
  -d '{"nodeId":"node3","available":true}'
```

## 11. Backend-Only Proof Points

Use these when someone asks where the real Raft behavior lives:

```bash
./scripts/test_backend.sh
```

Important test files:

```text
backend/tests/unit/replication_test.cpp
backend/tests/unit/failover_test.cpp
backend/tests/unit/snapshot_test.cpp
backend/tests/unit/raft_log_test.cpp
backend/tests/unit/storage_test.cpp
backend/tests/unit/rpc_mapper_test.cpp
```

What to say:

```text
The dashboard is for demonstration. The backend tests are the correctness
contract for the C++ Raft implementation.
```

## 12. Full Command Reference

Build backend:

```bash
./scripts/build_backend.sh
```

Test backend:

```bash
./scripts/test_backend.sh
```

Test control plane:

```bash
./scripts/test_control_plane.sh
```

Build UI:

```bash
./scripts/test_ui.sh
```

Run control plane:

```bash
./scripts/run_control_plane.sh
```

Run UI:

```bash
./scripts/run_ui.sh
```

Run full dev stack:

```bash
./scripts/run_dev.sh
```

Stop local dev stack:

```bash
./scripts/stop_dev.sh
```

Run all demos:

```bash
./scripts/demo_all.sh
```

Run individual demos:

```bash
./scripts/demo_leader_election.sh
./scripts/demo_log_replication.sh
./scripts/demo_follower_catchup.sh
./scripts/demo_leader_failover.sh
./scripts/demo_no_quorum.sh
./scripts/demo_network_partition.sh
./scripts/demo_snapshot_install.sh
./scripts/demo_control_plane.sh
```

Docker backend test:

```bash
./scripts/docker_test_backend.sh
```

Start current Docker shell cluster:

```bash
./scripts/run_cluster.sh
```

Stop Docker cluster:

```bash
./scripts/stop_cluster.sh
```

Reset Docker cluster data:

```bash
./scripts/reset_cluster.sh
```

## 13. Suggested Demo Order

Use this order for a clean 10 to 15 minute walkthrough:

1. Show `README.md` and explain the honest current state.
2. Run `./scripts/test_backend.sh`.
3. Start `./scripts/run_dev.sh`.
4. Open `http://127.0.0.1:5173`.
5. Run a normal `PUT`.
6. Show No Quorum.
7. Show Leader Failover.
8. Show Network Partition.
9. Show Snapshot Install.
10. Open `docs/INTERVIEW_GUIDE.md` and explain the limitations.

## 14. Common Troubleshooting

If the UI says the control plane is offline:

```bash
curl -s http://127.0.0.1:8090/health
```

If port `8090` is already in use, stop the old control-plane process and run:

```bash
./scripts/run_control_plane.sh
```

If port `5173` is already in use, Vite may choose another port. Check the
terminal output from:

```bash
./scripts/run_ui.sh
```

If Docker tests fail, confirm Docker is running:

```bash
docker version
```

If backend dependency detection fails, confirm Protobuf and gRPC C++ development
packages are installed.
