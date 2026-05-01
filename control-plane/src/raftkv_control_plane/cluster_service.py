from typing import Dict, List, Optional

from .models import ClusterEvent, ClusterSnapshot, CommandResult, NodeStatus


class ClusterService:
    def __init__(self) -> None:
        self._nodes: Dict[str, NodeStatus] = {}
        self._kv: Dict[str, str] = {}
        self._events: List[ClusterEvent] = []
        self._leader_id: Optional[str] = None
        self._next_log_index = 1
        self._next_event_sequence = 1
        self._snapshot_index = 0
        self._compacted_log_start_index = 1
        self.reset()

    def reset(self) -> ClusterSnapshot:
        self._nodes = {
            "node1": NodeStatus("node1", role="LEADER", term=1),
            "node2": NodeStatus("node2", role="FOLLOWER", term=1),
            "node3": NodeStatus("node3", role="FOLLOWER", term=1),
        }
        self._kv = {}
        self._events = []
        self._leader_id = "node1"
        self._next_log_index = 1
        self._next_event_sequence = 1
        self._snapshot_index = 0
        self._compacted_log_start_index = 1
        self._emit("BECAME_LEADER", "node1", "node1 became leader", 1)
        return self.snapshot()

    def snapshot(self) -> ClusterSnapshot:
        return ClusterSnapshot(
            leader_id=self._leader_id,
            majority=self.majority(),
            nodes=list(self._nodes.values()),
            kv=dict(self._kv),
            snapshot_index=self._snapshot_index,
            compacted_log_start_index=self._compacted_log_start_index,
        )

    def events(self) -> List[ClusterEvent]:
        return list(self._events)

    def majority(self) -> int:
        return (len(self._nodes) // 2) + 1

    def set_node_available(self, node_id: str, available: bool) -> ClusterSnapshot:
        node = self._require_node(node_id)
        node.available = available
        event_type = "NODE_STARTED" if available else "NODE_STOPPED"
        message = f"{node_id} {'started' if available else 'stopped'}"
        self._emit(event_type, node_id, message, node.term)

        if self._leader_id == node_id and not available:
            self._leader_id = None
            node.role = "FOLLOWER"
            self._emit("QUORUM_LOST", node_id, "leader unavailable", node.term)

        return self.snapshot()

    def partition_node(self, node_id: str) -> ClusterSnapshot:
        node = self._require_node(node_id)
        node.available = False
        self._emit("NETWORK_PARTITIONED", node_id, f"{node_id} isolated from majority", node.term)

        if self._leader_id == node_id:
            self._leader_id = None
            self._emit("QUORUM_LOST", node_id, "old leader isolated without quorum", node.term)
            self._elect_available_leader()

        return self.snapshot()

    def heal_node(self, node_id: str) -> ClusterSnapshot:
        node = self._require_node(node_id)
        node.available = True
        self._emit("NODE_STARTED", node_id, f"{node_id} rejoined cluster", node.term)

        if self._leader_id is not None and node.role == "LEADER" and node.node_id != self._leader_id:
            active_leader = self._nodes[self._leader_id]
            node.role = "FOLLOWER"
            node.term = max(node.term, active_leader.term)
            self._emit(
                "STALE_TERM_REJECTED",
                node_id,
                f"{node_id} stepped down after seeing active leader {active_leader.node_id}",
                node.term,
            )

        return self.snapshot()

    def run_demo(self, scenario: str) -> dict:
        normalized = scenario.lower().replace("_", "-")
        if normalized == "no-quorum":
            return self._demo_no_quorum()
        if normalized == "leader-failover":
            return self._demo_leader_failover()
        if normalized == "network-partition":
            return self._demo_network_partition()
        if normalized == "snapshot-install":
            return self._demo_snapshot_install()
        raise ValueError(f"unsupported demo scenario: {scenario}")

    def create_snapshot(self) -> ClusterSnapshot:
        commit_index = max((node.commit_index for node in self._nodes.values()), default=0)
        if commit_index == 0:
            raise ValueError("cannot snapshot before a committed entry exists")

        leader = self._current_leader()
        self._snapshot_index = commit_index
        self._compacted_log_start_index = commit_index + 1
        for node in self._nodes.values():
            if node.commit_index >= commit_index:
                node.last_log_index = max(node.last_log_index, commit_index)

        self._emit(
            "SNAPSHOT_CREATED",
            leader.node_id,
            f"snapshot captured committed state through index {commit_index}",
            leader.term,
            commit_index,
        )
        self._emit(
            "LOG_COMPACTED",
            leader.node_id,
            f"log entries before index {self._compacted_log_start_index} compacted",
            leader.term,
            commit_index,
        )
        return self.snapshot()

    def run_command(self, command: str, key: str, value: Optional[str] = None) -> CommandResult:
        normalized = command.upper()
        if normalized == "GET":
            return self._get(key)
        if normalized == "PUT":
            if value is None:
                raise ValueError("PUT requires value")
            return self._mutate("PUT", key, value)
        if normalized == "DELETE":
            return self._mutate("DELETE", key, None)
        raise ValueError(f"unsupported command: {command}")

    def _get(self, key: str) -> CommandResult:
        value = self._kv.get(key)
        return CommandResult(
            ok=value is not None,
            committed=False,
            index=0,
            acknowledgements=0,
            majority=self.majority(),
            message="found" if value is not None else "not found",
            value=value,
        )

    def _mutate(self, command: str, key: str, value: Optional[str]) -> CommandResult:
        if not key:
            raise ValueError("key must not be empty")

        leader = self._current_leader()
        alive_nodes = [node for node in self._nodes.values() if node.available]
        acknowledgements = len(alive_nodes)
        index = self._next_log_index
        self._next_log_index += 1

        leader.last_log_index = index
        self._emit("LOG_APPENDED", leader.node_id, f"{command} {key}", leader.term, index)

        committed = acknowledgements >= self.majority()
        if not committed:
            self._emit(
                "QUORUM_LOST",
                leader.node_id,
                f"write at index {index} did not reach quorum",
                leader.term,
                index,
            )
            return CommandResult(
                ok=False,
                committed=False,
                index=index,
                acknowledgements=acknowledgements,
                majority=self.majority(),
                message="quorum not reached",
            )

        for node in alive_nodes:
            node.last_log_index = max(node.last_log_index, index)
            node.commit_index = index
            node.last_applied = index

        if command == "PUT":
            self._kv[key] = value or ""
        elif command == "DELETE":
            self._kv.pop(key, None)

        self._emit(
            "ENTRY_COMMITTED",
            leader.node_id,
            f"entry {index} committed with {acknowledgements}/{len(self._nodes)} acknowledgements",
            leader.term,
            index,
        )
        self._emit("ENTRY_APPLIED", leader.node_id, f"{command} {key} applied", leader.term, index)

        return CommandResult(
            ok=True,
            committed=True,
            index=index,
            acknowledgements=acknowledgements,
            majority=self.majority(),
            message="committed",
        )

    def _current_leader(self) -> NodeStatus:
        if self._leader_id is None:
            self._elect_available_leader()
        if self._leader_id is None:
            raise RuntimeError("no leader available")

        leader = self._nodes[self._leader_id]
        if not leader.available:
            self._leader_id = None
            return self._current_leader()
        return leader

    def _demo_no_quorum(self) -> dict:
        self.reset()
        self.set_node_available("node2", False)
        self.set_node_available("node3", False)
        result = self.run_command("PUT", "payment:55", "success")
        return {
            "scenario": "no-quorum",
            "passed": not result.committed,
            "result": result.to_dict(),
            "cluster": self.snapshot().to_dict(),
            "events": [event.to_dict() for event in self.events()],
        }

    def _demo_leader_failover(self) -> dict:
        self.reset()
        first = self.run_command("PUT", "session:1", "active")
        old_leader = self._leader_id
        if old_leader is not None:
            self.set_node_available(old_leader, False)
        second = self.run_command("PUT", "session:2", "active")
        passed = first.committed and second.committed and self._kv.get("session:1") == "active"
        return {
            "scenario": "leader-failover",
            "passed": passed,
            "oldLeader": old_leader,
            "newLeader": self._leader_id,
            "firstWrite": first.to_dict(),
            "secondWrite": second.to_dict(),
            "cluster": self.snapshot().to_dict(),
            "events": [event.to_dict() for event in self.events()],
        }

    def _demo_network_partition(self) -> dict:
        self.reset()
        old_leader = self._leader_id
        if old_leader is None:
            raise RuntimeError("no leader available")

        self.partition_node(old_leader)
        write = self.run_command("PUT", "split:test", "new-leader-write")
        self.heal_node(old_leader)
        passed = write.committed and self._kv.get("split:test") == "new-leader-write"
        return {
            "scenario": "network-partition",
            "passed": passed,
            "oldLeader": old_leader,
            "newLeader": self._leader_id,
            "write": write.to_dict(),
            "cluster": self.snapshot().to_dict(),
            "events": [event.to_dict() for event in self.events()],
        }

    def _demo_snapshot_install(self) -> dict:
        self.reset()
        self.set_node_available("node3", False)
        first = self.run_command("PUT", "catalog:1", "available")
        second = self.run_command("PUT", "catalog:2", "reserved")
        snapshot = self.create_snapshot()
        self.set_node_available("node3", True)

        lagging = self._nodes["node3"]
        lagging.commit_index = self._snapshot_index
        lagging.last_applied = self._snapshot_index
        lagging.last_log_index = max(lagging.last_log_index, self._snapshot_index)
        self._emit(
            "SNAPSHOT_INSTALLED",
            lagging.node_id,
            f"{lagging.node_id} installed snapshot through index {self._snapshot_index}",
            lagging.term,
            self._snapshot_index,
        )

        passed = (
            first.committed
            and second.committed
            and snapshot.snapshot_index == 2
            and lagging.commit_index == self._snapshot_index
            and self._kv.get("catalog:1") == "available"
            and self._kv.get("catalog:2") == "reserved"
        )
        return {
            "scenario": "snapshot-install",
            "passed": passed,
            "snapshotIndex": self._snapshot_index,
            "compactedLogStartIndex": self._compacted_log_start_index,
            "cluster": self.snapshot().to_dict(),
            "events": [event.to_dict() for event in self.events()],
        }

    def _elect_available_leader(self) -> None:
        alive = [node for node in self._nodes.values() if node.available]
        if len(alive) < self.majority():
            return

        new_term = max(node.term for node in self._nodes.values()) + 1
        for node in alive:
            node.term = new_term
            node.role = "FOLLOWER"

        leader = alive[0]
        leader.role = "LEADER"
        self._leader_id = leader.node_id
        self._emit("BECAME_LEADER", leader.node_id, f"{leader.node_id} became leader", new_term)

    def _require_node(self, node_id: str) -> NodeStatus:
        node = self._nodes.get(node_id)
        if node is None:
            raise KeyError(f"unknown node: {node_id}")
        return node

    def _emit(self, event_type: str, node_id: str, message: str, term: int, log_index: int = 0) -> None:
        self._events.append(
            ClusterEvent(
                sequence=self._next_event_sequence,
                type=event_type,
                node_id=node_id,
                message=message,
                term=term,
                log_index=log_index,
            )
        )
        self._next_event_sequence += 1
