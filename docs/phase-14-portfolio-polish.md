# Phase 14: CI, Polish, And Portfolio Readiness

Branch: `chore/portfolio-polish`

Goal: make the repository easy to review from a fresh clone.

## What Changed

- Refreshed the root README to describe the real current state.
- Updated architecture and testing docs.
- Added a demo walkthrough.
- Added an interview guide with honest tradeoffs and likely questions.
- Added GitHub Actions for backend, control-plane, UI, and demos.

## Why This Matters

Projects often fail review because the code and the story drift apart. This
phase aligns them:

- local commands match CI jobs
- docs state what is implemented today
- limitations are explicit
- demos have a clear walkthrough path

## Verification

```bash
./scripts/test_backend.sh
./scripts/test_control_plane.sh
./scripts/test_ui.sh
./scripts/demo_all.sh
```

## Next Good Feature Branch

```text
feat/live-grpc-node-server
```

That branch should replace the simulated control-plane cluster with real C++
node RPCs one path at a time.
