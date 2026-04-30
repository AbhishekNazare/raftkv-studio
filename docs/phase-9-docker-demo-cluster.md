# Phase 9: Docker Demo Cluster

Branch: `feat/docker-demo-cluster`

## What This Phase Adds

This phase makes the backend containerized and gives the project a repeatable
cluster shell:

- `Dockerfile.backend`
- `docker-compose.yml`
- Three named node services: `node1`, `node2`, `node3`
- Persistent Docker volumes for each node
- Node config files under `backend/config/`
- Cluster lifecycle scripts
- Demo scripts for the behaviors already proven by backend tests

## Why The Docker Nodes Are Still Standalone

The project has Protobuf/gRPC contracts, but it does not yet have a real
long-running gRPC node server. This phase avoids pretending otherwise.

The Docker containers currently run `raftkv-node --standalone`, which gives each
container a stable node identity and data directory. Live cross-container Raft
traffic will be added when the node server is implemented.

## Commands

Build and test inside Docker:

```bash
./scripts/docker_test_backend.sh
```

Start the 3-node container shell:

```bash
./scripts/run_cluster.sh
```

Stop the cluster:

```bash
./scripts/stop_cluster.sh
```

Reset volumes:

```bash
./scripts/reset_cluster.sh
```

Run all current demo checks:

```bash
./scripts/demo_all.sh
```

## Current Demo Coverage

The scripts currently call the backend test suite and print the behavior being
proved:

- leader election safety
- quorum log replication
- follower catch-up
- leader failover
- no-quorum rejection

These are deterministic in-process demos today. They become live Docker demos
after the gRPC node server phase.

## Next Phase

Next branch:

```text
feat/control-plane-api
```

Before a fully live UI, the project needs a control plane that can expose
cluster status, command forwarding, and event streams.
