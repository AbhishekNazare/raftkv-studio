# Interview Guide

Use this guide to explain RaftKV Studio without overselling it.

## Short Pitch

RaftKV Studio is a C++20 Raft learning project built like a production system:
small phases, tests beside each feature, demo scripts, a control-plane API, and
a React observability dashboard.

The strongest part is not that it is a finished database. The strongest part is
that each distributed-systems behavior is isolated, tested, documented, and
demonstrable.

## What To Show First

1. `README.md` for the project story.
2. `TESTING.md` for the verification matrix.
3. `backend/tests/unit/replication_test.cpp` for quorum behavior.
4. `backend/tests/unit/failover_test.cpp` for catch-up and failover.
5. `backend/tests/unit/snapshot_test.cpp` for compaction and snapshot install.
6. The dashboard for a visual walkthrough.

## Strong Technical Points

- The project separates consensus behavior from UI/demo concerns.
- The in-process cluster makes Raft behavior deterministic and testable.
- Quorum commit rules are tested directly.
- No-quorum and partition demos prove negative behavior, not only happy paths.
- Snapshot compaction preserves the last included term for future log matching.
- Docs track what changed in each phase, so the Git history is useful for
  learning and review.

## Honest Limitations

- The dashboard currently talks to a deterministic Python control-plane
  simulation.
- The C++ backend has protobuf/gRPC contracts and mapper tests, but no complete
  live multi-process Raft server loop yet.
- Snapshot files are not durable artifacts yet.
- There is no membership-change protocol.
- Reads are not proven linearizable.
- There is no client request deduplication.
- The storage layer is educational and not tuned like a production database.

## Good Answers To Likely Questions

Question: Why use an in-process cluster first?

Answer: It removes timing and networking noise so consensus rules can be tested
deterministically. Once those contracts are stable, the live RPC layer has
clear behavior to preserve.

Question: What does the control plane prove?

Answer: It proves the API, UI workflows, event model, and demo story. It does
not claim to prove live distributed execution. Backend C++ tests prove the Raft
behaviors currently implemented.

Question: What would you build next?

Answer: Wire the C++ gRPC node server end to end, add install-snapshot RPC,
persist snapshot files, then add Playwright UI tests against the running
control plane.

Question: What was the hardest correctness detail?

Answer: Keeping log matching meaningful after compaction. Once old entries are
removed, the node still needs the snapshot boundary index and term so future
append consistency checks can compare against the last included entry.
