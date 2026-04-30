# RaftKV Studio Phase Plan

This project should be built as a sequence of small, reviewable branches. Each
phase should leave the repository in a working state, with tests or demo commands
that prove the feature.

## Branch and Commit Rules

- One feature branch per phase.
- Prefer small commits that each explain one engineering decision.
- Every branch should end with a README/docs update explaining what was learned.
- Tests are part of the feature, not a later cleanup task.
- Demo scripts should print what they prove, not just run commands.

Recommended commit style:

```text
chore: scaffold project layout
test: add raft log unit tests
feat: append and read raft log entries
docs: explain replicated write path
```

## Phase 0: Project Foundation

Branch: `chore/project-foundation`

Goal: Turn the current project brief into a clean repository with build, docs,
and development conventions.

Build:

- Create root `README.md` from the current project brief.
- Add `DESIGN.md`, `TESTING.md`, and this phase plan.
- Add `.gitignore`.
- Add root folder structure: `backend/`, `control-plane/`, `ui/`, `scripts/`,
  `docs/`, and `.github/workflows/`.
- Decide the first implementation path: backend core first, UI mock second.

Learn:

- What RaftKV Studio is supposed to demonstrate.
- Why branch history matters for a portfolio systems project.
- How to keep architecture, testing, and demos aligned.

Suggested commits:

```text
chore: initialize raftkv studio repository
docs: add phased implementation roadmap
docs: capture architecture and testing strategy
```

Exit criteria:

- Repo has a clean structure.
- Docs explain what will be built and in what order.
- Git branch and commit strategy is documented.

## Phase 1: C++ Backend Skeleton

Branch: `feat/backend-cpp-skeleton`

Goal: Create a compilable C++20 backend with CMake and GoogleTest.

Build:

- Add `backend/CMakeLists.txt`.
- Add core headers for IDs, terms, indexes, status, and result handling.
- Add a minimal `raftkv-node` executable.
- Add a minimal GoogleTest target.
- Add `scripts/build_backend.sh` and `scripts/test_backend.sh`.

Learn:

- How CMake organizes libraries, executables, and tests.
- Why distributed systems projects need clear domain types.
- How to keep a systems codebase testable from day one.

Suggested commits:

```text
build: add c++20 cmake backend skeleton
chore: add backend build and test scripts
test: add first google test smoke test
```

Exit criteria:

- `cmake --build` succeeds.
- Unit test binary runs.
- CI can build the backend.

## Phase 2: In-Memory KV State Machine

Branch: `feat/in-memory-kv-state-machine`

Goal: Implement deterministic `PUT`, `GET`, and `DELETE` behavior before Raft.

Build:

- Add `Command` and command codec.
- Add an in-memory state machine.
- Add unit tests for put/get/delete/idempotent behavior.
- Add a temporary local CLI or test harness for commands.

Learn:

- Raft replicates commands, not arbitrary database mutations.
- The state machine must be deterministic.
- Reads and writes have different correctness requirements.

Suggested commits:

```text
feat: add kv command model and codec
feat: implement in-memory kv state machine
test: cover put get delete state transitions
docs: explain command application model
```

Exit criteria:

- State machine tests pass.
- Command serialization is covered by tests.

## Phase 3: Raft Log Model

Branch: `feat/raft-log-core`

Goal: Implement an in-memory Raft log with append, lookup, conflict handling,
and commit tracking.

Build:

- Add `LogEntry`, `RaftLog`, term/index helpers.
- Append entries with monotonically increasing indexes.
- Validate `prevLogIndex` and `prevLogTerm`.
- Delete conflicting suffixes when leader log wins.
- Add unit tests for log matching and conflict replacement.

Learn:

- Why Raft log matching prevents divergent histories.
- Why entries are committed only after quorum, not just after append.
- How uncommitted entries can be overwritten safely.

Suggested commits:

```text
feat: add raft log entry model
feat: implement append and lookup operations
feat: handle conflicting raft log suffixes
test: cover raft log matching rules
```

Exit criteria:

- Raft log unit tests pass.
- Conflict behavior is documented.

## Phase 4: Single-Process Raft Simulation

Branch: `feat/in-process-raft-cluster`

Goal: Build a deterministic 3-node Raft cluster in one process before adding
gRPC or RocksDB.

Build:

- Add `RaftNode` role, term, vote, leader, commit, and apply state.
- Add in-process `RequestVote` and `AppendEntries` calls.
- Add randomized election timeout abstraction, with deterministic tests.
- Add heartbeat loop simulation.
- Add cluster test harness.

Learn:

- Follower, candidate, and leader transitions.
- Why randomized election timeouts reduce split votes.
- Why a leader must step down when it sees a higher term.

Suggested commits:

```text
feat: add raft node role and term state
feat: implement request vote handling
feat: implement append entries heartbeat handling
test: elect exactly one leader in simulated cluster
```

Exit criteria:

- Test proves exactly one leader is elected.
- Test proves stale terms are rejected.

## Phase 5: Replication and Quorum Commit

Branch: `feat/quorum-log-replication`

Goal: Replicate client commands through the simulated Raft cluster and commit
only after majority acknowledgment.

Build:

- Leader appends client command locally.
- Leader replicates to followers.
- Track `nextIndex` and `matchIndex`.
- Advance `commitIndex` when majority has an entry from the current term.
- Apply committed entries to KV state machines.

Learn:

- Majority commit is the core safety mechanism.
- Followers may lag behind without blocking progress.
- Applied state is separate from the durable log.

Suggested commits:

```text
feat: replicate log entries to followers
feat: track follower next index and match index
feat: commit entries after majority replication
test: prove put is applied after quorum commit
```

Exit criteria:

- Test proves writes commit with 2/3 nodes.
- Test proves writes fail or remain uncommitted without quorum.

## Phase 6: Follower Catch-Up and Failover

Branch: `feat/follower-catchup-failover`

Goal: Demonstrate the first serious distributed systems scenarios.

Build:

- Stop or isolate one follower in the simulation.
- Commit entries with the remaining majority.
- Bring follower back and catch it up.
- Crash leader and elect a new leader.
- Preserve committed entries across failover inside the simulation.

Learn:

- Why Raft tolerates missing followers.
- How `nextIndex` backtracking repairs lagging logs.
- How leader completeness protects committed data.

Suggested commits:

```text
test: cover follower missing committed entries
feat: catch up lagging followers with append entries
test: cover leader failover after committed write
docs: add follower catch-up walkthrough
```

Exit criteria:

- Test proves follower catch-up.
- Test proves leader failover keeps committed data readable.

## Phase 7: Persistence Layer

Branch: `feat/persistent-raft-storage`

Goal: Add crash-safe storage behind the existing interfaces.

Build:

- Define `LogStore`, `MetadataStore`, and `KVStore` interfaces.
- Add file-backed storage first or RocksDB if dependency setup is ready.
- Persist `currentTerm`, `votedFor`, log entries, and KV state.
- Add restart tests.

Learn:

- Followers must persist log entries before acknowledging.
- `currentTerm` and `votedFor` must survive restarts.
- Applied KV state and Raft log have different recovery roles.

Suggested commits:

```text
feat: add storage interfaces for raft state
feat: persist term vote and log entries
feat: persist kv state machine data
test: recover committed data after node restart
```

Exit criteria:

- Restart test passes.
- Durable ACK rule is documented.

## Phase 8: gRPC and Protobuf Networking

Branch: `feat/grpc-raft-rpc`

Goal: Move from in-process calls to real process-to-process RPC.

Build:

- Add `raft.proto`, `kv.proto`, `debug.proto`, and `event.proto`.
- Add gRPC server and clients.
- Run three local node processes.
- Add `kvctl` commands: `put`, `get`, `delete`, `status`, `leader`.

Learn:

- How Raft RPC maps onto network boundaries.
- Why deadlines, retries, and stale-term handling matter.
- How a client discovers and follows the current leader.

Suggested commits:

```text
feat: define raft and kv protobuf contracts
feat: add grpc raft service and client
feat: add kvctl command client
test: replicate write across local node processes
```

Exit criteria:

- Three local processes elect a leader.
- `kvctl put/get` works through the leader.

## Phase 9: Docker Cluster and Demo Scripts

Branch: `feat/docker-demo-cluster`

Goal: Make the project easy to run and demo.

Build:

- Add Dockerfiles and `docker-compose.yml`.
- Add node config files.
- Add scripts for cluster lifecycle.
- Add demo scripts for leader election, log replication, no quorum,
  follower catch-up, and leader failover.

Learn:

- How to make distributed behavior reproducible.
- Why demo scripts should show expected and actual behavior.
- How Docker helps simulate node failure.

Suggested commits:

```text
build: containerize raftkv node
chore: add three node docker compose cluster
feat: add kvctl cluster inspection commands
test: add demo scripts for core raft scenarios
```

Exit criteria:

- `docker compose up` starts a 3-node cluster.
- Demo scripts prove the main Raft behaviors.

## Phase 10: Control Plane API

Branch: `feat/control-plane-api`

Goal: Add the HTTP/WebSocket gateway used by the UI.

Build:

- Add cluster, command, fault, demo, and snapshot routes.
- Add node debug clients.
- Add WebSocket event streaming.
- Expose cluster status, logs, KV state, and event history.

Learn:

- Why a UI should not talk directly to every Raft node.
- How observability data differs from client command data.
- How live event streams make distributed systems understandable.

Suggested commits:

```text
feat: scaffold control plane api
feat: expose cluster status and command endpoints
feat: stream raft events over websocket
test: cover control plane command forwarding
```

Exit criteria:

- API returns cluster status.
- UI can subscribe to events.

## Phase 11: React Observability Dashboard

Branch: `feat/raft-observability-ui`

Goal: Build the first complete visual demo experience.

Build:

- Add Vite React TypeScript app.
- Add app shell, sidebar, status bar, and dashboard layout.
- Add cluster topology, node details, terminal, event timeline, replicated log
  table, and KV state viewer.
- Connect to control plane HTTP and WebSocket APIs.

Learn:

- How to turn backend state into explainable UI.
- How log replication, commit index, and node roles should be visualized.
- How a demo UI differs from a marketing page.

Suggested commits:

```text
feat: scaffold react dashboard
feat: add cluster topology and node details
feat: add command terminal and event timeline
feat: render replicated log and kv state views
```

Exit criteria:

- UI shows live cluster status and events.
- A user can run a `put` and watch it replicate.

## Phase 12: Fault Injection and Guided Demos

Branch: `feat/fault-injection-demos`

Goal: Make failure scenarios visible and repeatable from UI and scripts.

Build:

- Add node stop/start controls.
- Add network partition controls.
- Add message delay/drop/duplicate/reorder controls if feasible.
- Add guided demo runner steps.
- Add scripts for no quorum, split-brain prevention, and network partition.

Learn:

- Raft safety depends on majority, terms, and log freshness.
- Old leaders may exist temporarily, but cannot commit.
- The best demos prove negative behavior too: unsafe writes do not happen.

Suggested commits:

```text
feat: add fault injection api
feat: add network partition controls
feat: add guided demo runner
test: prove old leader cannot commit after partition
```

Exit criteria:

- UI can demonstrate no quorum and split-brain prevention.
- Scripted demos match UI behavior.

## Phase 13: Snapshots and Log Compaction

Branch: `feat/snapshots-log-compaction`

Goal: Add production-style log management and lagging follower snapshot install.

Build:

- Add snapshot manager.
- Persist snapshot metadata.
- Compact logs covered by snapshots.
- Add `InstallSnapshot` RPC.
- Add snapshot status UI.
- Add snapshot recovery and snapshot install demos.

Learn:

- Applied logs are not immediately disposable.
- Snapshots are a recovery and compaction mechanism.
- A far-behind follower may need a snapshot instead of old log entries.

Suggested commits:

```text
feat: create durable kv snapshots
feat: compact raft log after snapshot
feat: install snapshots on lagging followers
test: recover follower from compacted leader log
```

Exit criteria:

- Snapshot tests pass.
- Far-behind follower installs snapshot and catches up.

## Phase 14: CI, Polish, and Portfolio Readiness

Branch: `chore/portfolio-polish`

Goal: Make the project presentable, reproducible, and interview-ready.

Build:

- Add GitHub Actions for backend, UI, and integration tests.
- Add screenshots or short demo recordings.
- Finalize README, DESIGN, TESTING, and interview guide.
- Add benchmark notes and limitations.

Learn:

- How to communicate engineering tradeoffs honestly.
- How to present a complex system without overselling it.
- How to make reviewers trust the project quickly.

Suggested commits:

```text
ci: add backend and ui workflows
docs: add demo guide and testing matrix
docs: add architecture tradeoffs and limitations
chore: prepare portfolio release
```

Exit criteria:

- Fresh clone instructions work.
- Core demos work.
- README clearly tells the project story.
