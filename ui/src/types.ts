export type NodeRole = "LEADER" | "FOLLOWER" | "CANDIDATE";

export interface NodeStatus {
  nodeId: string;
  role: NodeRole;
  term: number;
  available: boolean;
  commitIndex: number;
  lastApplied: number;
  lastLogIndex: number;
}

export interface ClusterSnapshot {
  leaderId: string | null;
  majority: number;
  nodes: NodeStatus[];
  kv: Record<string, string>;
}

export interface ClusterEvent {
  sequence: number;
  type: string;
  nodeId: string;
  message: string;
  term: number;
  logIndex: number;
}

export interface CommandResult {
  ok: boolean;
  committed: boolean;
  index: number;
  acknowledgements: number;
  majority: number;
  message: string;
  value?: string;
}

