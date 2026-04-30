#!/usr/bin/env bash
set -euo pipefail

./scripts/demo_leader_election.sh
./scripts/demo_log_replication.sh
./scripts/demo_follower_catchup.sh
./scripts/demo_leader_failover.sh
./scripts/demo_no_quorum.sh
./scripts/demo_network_partition.sh
./scripts/demo_control_plane.sh
