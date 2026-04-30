from dataclasses import dataclass, field
from typing import Dict, List, Optional


@dataclass
class NodeStatus:
    node_id: str
    role: str = "FOLLOWER"
    term: int = 0
    available: bool = True
    commit_index: int = 0
    last_applied: int = 0
    last_log_index: int = 0

    def to_dict(self) -> dict:
        return {
            "nodeId": self.node_id,
            "role": self.role,
            "term": self.term,
            "available": self.available,
            "commitIndex": self.commit_index,
            "lastApplied": self.last_applied,
            "lastLogIndex": self.last_log_index,
        }


@dataclass
class ClusterEvent:
    sequence: int
    type: str
    node_id: str
    message: str
    term: int
    log_index: int = 0

    def to_dict(self) -> dict:
        return {
            "sequence": self.sequence,
            "type": self.type,
            "nodeId": self.node_id,
            "message": self.message,
            "term": self.term,
            "logIndex": self.log_index,
        }


@dataclass
class CommandResult:
    ok: bool
    committed: bool
    index: int
    acknowledgements: int
    majority: int
    message: str
    value: Optional[str] = None

    def to_dict(self) -> dict:
        body = {
            "ok": self.ok,
            "committed": self.committed,
            "index": self.index,
            "acknowledgements": self.acknowledgements,
            "majority": self.majority,
            "message": self.message,
        }
        if self.value is not None:
            body["value"] = self.value
        return body


@dataclass
class ClusterSnapshot:
    leader_id: Optional[str]
    majority: int
    nodes: List[NodeStatus]
    kv: Dict[str, str] = field(default_factory=dict)

    def to_dict(self) -> dict:
        return {
            "leaderId": self.leader_id,
            "majority": self.majority,
            "nodes": [node.to_dict() for node in self.nodes],
            "kv": dict(self.kv),
        }

