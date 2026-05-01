# RaftKV Studio Testing Strategy

Testing is part of the project story. Each layer proves a different claim about
the system.

## Local Test Matrix

| Command | What It Checks |
| --- | --- |
| `./scripts/test_backend.sh` | C++ build plus all CTest unit and integration-style tests. |
| `./scripts/test_control_plane.sh` | HTTP control-plane behavior, commands, faults, and demos. |
| `./scripts/test_ui.sh` | TypeScript compile and Vite production build. |
| `./scripts/demo_all.sh` | Runnable documentation for the main consensus scenarios. |
| `./scripts/docker_test_backend.sh` | Backend build and tests inside the Docker image. |

## Backend Tests

The backend tests cover:

- version/smoke wiring
- command encoding and decoding
- KV state-machine PUT, GET, DELETE, and snapshot behavior
- Raft log append, matching, conflicts, and compaction
- candidate election and leader tracking
- quorum commit behavior
- follower catch-up
- leader failover
- file-backed storage primitives
- gRPC/protobuf request mapping
- snapshot creation and far-behind follower install

Run:

```bash
./scripts/test_backend.sh
```

## Control-Plane Tests

The control-plane tests start an in-process HTTP server on a random local port
and verify:

- health checks
- cluster status
- command flow
- no-quorum rejection
- event recording
- leader failover demo
- network partition demo
- snapshot create route
- snapshot install demo

Run:

```bash
./scripts/test_control_plane.sh
```

## UI Tests

The UI currently uses build-time verification:

- TypeScript compile
- Vite production bundle

Run:

```bash
./scripts/test_ui.sh
```

The next useful UI testing step would be a small Playwright suite that starts
the control plane and verifies the dashboard can load cluster state, run a PUT,
and trigger a guided demo.

## Demo Scripts

Demo scripts are executable documentation. They should be understandable even
when read in a terminal log.

```bash
./scripts/demo_leader_election.sh
./scripts/demo_log_replication.sh
./scripts/demo_follower_catchup.sh
./scripts/demo_leader_failover.sh
./scripts/demo_no_quorum.sh
./scripts/demo_network_partition.sh
./scripts/demo_snapshot_install.sh
./scripts/demo_control_plane.sh
./scripts/demo_all.sh
```

## CI Expectations

GitHub Actions run the same checks in separate jobs:

- backend C++ tests
- control-plane tests
- UI build
- demo scripts

CI is intentionally simple. A reviewer should be able to map each CI job back to
one local command.
