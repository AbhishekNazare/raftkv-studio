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
        self._emit("BECAME_LEADER", "node1", "node1 became leader", 1)
        return self.snapshot()

    def snapshot(self) -> ClusterSnapshot:
        return ClusterSnapshot(
            leader_id=self._leader_id,
            majority=self.majority(),
            nodes=list(self._nodes.values()),
            kv=dict(self._kv),
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

    def _elect_available_leader(self) -> None:
        alive = [node for node in self._nodes.values() if node.available]
        if len(alive) < self.majority():
            return

        new_term = max(node.term for node in self._nodes.values()) + 1
        for node in self._nodes.values():
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

