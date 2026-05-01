import React, { useEffect, useMemo, useState } from "react";
import { createRoot } from "react-dom/client";
import {
  createSnapshot,
  getCluster,
  getEvents,
  healNode,
  partitionNode,
  resetDemo,
  runDemoScenario,
  sendCommand,
  setNodeAvailability
} from "./api";
import type { ClusterEvent, ClusterSnapshot, CommandResult, NodeStatus } from "./types";
import "./styles.css";

const emptyCluster: ClusterSnapshot = {
  leaderId: null,
  majority: 2,
  nodes: [],
  kv: {},
  snapshotIndex: 0,
  compactedLogStartIndex: 1
};

const demoScenarios = [
  { id: "no-quorum", label: "No Quorum", tone: "danger" },
  { id: "leader-failover", label: "Leader Failover", tone: "warning" },
  { id: "network-partition", label: "Partition Safety", tone: "signal" },
  { id: "snapshot-install", label: "Snapshot Install", tone: "success" }
];

function App() {
  const [cluster, setCluster] = useState<ClusterSnapshot>(emptyCluster);
  const [events, setEvents] = useState<ClusterEvent[]>([]);
  const [apiOnline, setApiOnline] = useState(false);
  const [selectedNode, setSelectedNode] = useState<string>("node1");
  const [command, setCommand] = useState("PUT");
  const [key, setKey] = useState("user:1");
  const [value, setValue] = useState("Abhishek");
  const [lastResult, setLastResult] = useState<CommandResult | null>(null);
  const [activity, setActivity] = useState("Waiting for cluster telemetry.");
  const [busyAction, setBusyAction] = useState<string | null>(null);
  const [error, setError] = useState<string | null>(null);

  async function refresh() {
    try {
      const [clusterData, eventData] = await Promise.all([getCluster(), getEvents()]);
      setCluster(clusterData);
      setEvents(eventData);
      setApiOnline(true);
      setError(null);
      if (!clusterData.nodes.some((node) => node.nodeId === selectedNode)) {
        setSelectedNode(clusterData.nodes[0]?.nodeId ?? "node1");
      }
    } catch (exc) {
      setApiOnline(false);
      setError(exc instanceof Error ? exc.message : "control plane unavailable");
    }
  }

  useEffect(() => {
    refresh();
    const timer = window.setInterval(refresh, 1800);
    return () => window.clearInterval(timer);
  }, []);

  const selected = useMemo(
    () => cluster.nodes.find((node) => node.nodeId === selectedNode) ?? cluster.nodes[0],
    [cluster.nodes, selectedNode]
  );

  const leader = useMemo(
    () => cluster.nodes.find((node) => node.nodeId === cluster.leaderId),
    [cluster.nodes, cluster.leaderId]
  );

  const health = useMemo(() => {
    const available = cluster.nodes.filter((node) => node.available).length;
    const quorum = available >= cluster.majority;
    return { available, quorum };
  }, [cluster.nodes, cluster.majority]);

  async function runAction(label: string, action: () => Promise<void>) {
    setBusyAction(label);
    setError(null);
    try {
      await action();
    } catch (exc) {
      const message = exc instanceof Error ? exc.message : "action failed";
      setError(message);
      setActivity(message);
    } finally {
      setBusyAction(null);
      await refresh();
    }
  }

  async function runCommand() {
    await runAction("command", async () => {
      const result = await sendCommand(command, key, command === "PUT" ? value : undefined);
      setLastResult(result);
      setActivity(
        `${command} ${key} ${result.committed ? "committed" : "rejected"} with ${result.acknowledgements}/${cluster.nodes.length || 3} acknowledgements`
      );
    });
  }

  async function reset() {
    await runAction("reset", async () => {
      await resetDemo();
      setLastResult(null);
      setActivity("Demo state reset. node1 is leader again.");
    });
  }

  async function snapshotNow() {
    await runAction("snapshot", async () => {
      const snapshot = await createSnapshot();
      setActivity(`Snapshot created through log index ${snapshot.snapshotIndex}.`);
    });
  }

  async function runScenario(scenario: string) {
    await runAction(scenario, async () => {
      const result = (await runDemoScenario(scenario)) as { passed?: boolean; scenario?: string };
      setActivity(`${result.scenario ?? scenario}: ${result.passed ? "passed" : "needs attention"}`);
    });
  }

  async function toggleNode(node: NodeStatus) {
    await runAction(`${node.nodeId}-toggle`, async () => {
      await setNodeAvailability(node.nodeId, !node.available);
      setActivity(`${node.nodeId} ${node.available ? "stopped" : "started"}.`);
    });
  }

  async function isolate(node: NodeStatus) {
    await runAction(`${node.nodeId}-partition`, async () => {
      await partitionNode(node.nodeId);
      setActivity(`${node.nodeId} isolated from the majority.`);
    });
  }

  async function heal(node: NodeStatus) {
    await runAction(`${node.nodeId}-heal`, async () => {
      await healNode(node.nodeId);
      setActivity(`${node.nodeId} healed and rejoined.`);
    });
  }

  return (
    <main className="app-shell">
      <section className="hero-console">
        <div className="brand-line">
          <span className="brand-mark">RK</span>
          <div>
            <h1>RaftKV Studio</h1>
            <p>Live consensus observability console</p>
          </div>
        </div>

        <div className="hero-metrics" aria-label="Cluster summary">
          <StatusPill online={apiOnline} />
          <Metric label="Leader" value={cluster.leaderId ?? "none"} />
          <Metric label="Available" value={`${health.available}/${cluster.nodes.length || 3}`} />
          <Metric label="Majority" value={cluster.majority} />
          <Metric label="Snapshot" value={cluster.snapshotIndex} />
        </div>

        <div className="hero-actions">
          <button onClick={refresh} disabled={busyAction !== null}>Refresh</button>
          <button className="primary" onClick={reset} disabled={!apiOnline || busyAction !== null}>
            Reset Demo
          </button>
        </div>
      </section>

      <section className="status-strip">
        <div className={`quorum-banner ${health.quorum ? "healthy" : "risk"}`}>
          <strong>{health.quorum ? "Quorum available" : "Quorum lost"}</strong>
          <span>{activity}</span>
        </div>
        {error && <div className="error-banner">{error}</div>}
      </section>

      <section className="dashboard-grid">
        <section className="surface topology-surface">
          <div className="surface-header">
            <div>
              <p className="eyebrow">cluster topology</p>
              <h2>{leader ? `${leader.nodeId} is coordinating writes` : "No active leader"}</h2>
            </div>
            <span className="live-badge">live</span>
          </div>

          <ClusterMap
            cluster={cluster}
            selectedNode={selected?.nodeId ?? ""}
            onSelect={setSelectedNode}
          />
        </section>

        <section className="surface command-surface">
          <div className="surface-header">
            <div>
              <p className="eyebrow">client command</p>
              <h2>Run KV operations</h2>
            </div>
            <span>{lastResult ? lastResult.message : "ready"}</span>
          </div>

          <div className="command-grid">
            <select value={command} onChange={(event) => setCommand(event.target.value)}>
              <option>PUT</option>
              <option>GET</option>
              <option>DELETE</option>
            </select>
            <input value={key} onChange={(event) => setKey(event.target.value)} aria-label="Key" />
            <input
              value={value}
              onChange={(event) => setValue(event.target.value)}
              aria-label="Value"
              disabled={command !== "PUT"}
            />
            <button className="primary" onClick={runCommand} disabled={!apiOnline || busyAction !== null}>
              Run
            </button>
          </div>

          <div className="terminal-output">
            <span>raftkv&gt; {command.toLowerCase()} {key}{command === "PUT" ? ` ${value}` : ""}</span>
            <strong>{lastResult ? `${lastResult.message} at index ${lastResult.index}` : "waiting for command"}</strong>
            {lastResult && (
              <span>
                ack {lastResult.acknowledgements}/{cluster.nodes.length || 3}, committed {String(lastResult.committed)}
              </span>
            )}
          </div>
        </section>

        <section className="surface node-surface">
          <div className="surface-header">
            <div>
              <p className="eyebrow">selected node</p>
              <h2>{selected?.nodeId ?? "No node"}</h2>
            </div>
            {selected && <span>{selected.available ? "available" : "unavailable"}</span>}
          </div>

          {selected ? (
            <>
              <div className="metric-grid">
                <Metric label="Role" value={selected.role} />
                <Metric label="Term" value={selected.term} />
                <Metric label="Commit" value={selected.commitIndex} />
                <Metric label="Applied" value={selected.lastApplied} />
                <Metric label="Last Log" value={selected.lastLogIndex} />
                <Metric label="Snapshot" value={cluster.snapshotIndex} />
              </div>
              <div className="node-actions">
                <button onClick={() => toggleNode(selected)} disabled={!apiOnline || busyAction !== null}>
                  {selected.available ? "Stop Node" : "Start Node"}
                </button>
                <button onClick={() => isolate(selected)} disabled={!apiOnline || !selected.available || busyAction !== null}>
                  Isolate
                </button>
                <button onClick={() => heal(selected)} disabled={!apiOnline || busyAction !== null}>
                  Heal
                </button>
              </div>
            </>
          ) : (
            <p className="empty">Start the control plane to load nodes.</p>
          )}
        </section>

        <section className="surface demo-surface">
          <div className="surface-header">
            <div>
              <p className="eyebrow">guided demos</p>
              <h2>Failure modes</h2>
            </div>
            <span>{busyAction ? "running" : "idle"}</span>
          </div>

          <div className="demo-grid">
            {demoScenarios.map((scenario) => (
              <button
                key={scenario.id}
                className={`demo-card ${scenario.tone}`}
                onClick={() => runScenario(scenario.id)}
                disabled={!apiOnline || busyAction !== null}
              >
                <span>{scenario.label}</span>
                <small>{scenario.id}</small>
              </button>
            ))}
          </div>
        </section>

        <section className="surface snapshot-surface">
          <div className="surface-header">
            <div>
              <p className="eyebrow">storage pressure</p>
              <h2>Snapshots</h2>
            </div>
            <span>compaction</span>
          </div>

          <div className="snapshot-meter" style={{ "--snapshot-progress": `${Math.min(cluster.snapshotIndex * 18, 100)}%` } as React.CSSProperties}>
            <span />
          </div>
          <div className="metric-grid two">
            <Metric label="Snapshot Index" value={cluster.snapshotIndex} />
            <Metric label="Log Starts At" value={cluster.compactedLogStartIndex} />
          </div>
          <button
            className="primary full-width"
            onClick={snapshotNow}
            disabled={!apiOnline || busyAction !== null || cluster.nodes.every((node) => node.commitIndex === 0)}
          >
            Create Snapshot
          </button>
        </section>

        <section className="surface event-surface">
          <div className="surface-header">
            <div>
              <p className="eyebrow">event stream</p>
              <h2>Timeline</h2>
            </div>
            <span>{events.length} events</span>
          </div>
          <div className="event-list">
            {events.slice().reverse().slice(0, 14).map((event) => (
              <EventRow key={event.sequence} event={event} />
            ))}
          </div>
        </section>

        <section className="surface state-surface">
          <div className="surface-header">
            <div>
              <p className="eyebrow">state machine</p>
              <h2>KV State</h2>
            </div>
            <span>{Object.keys(cluster.kv).length} keys</span>
          </div>
          <div className="kv-list">
            {Object.entries(cluster.kv).length === 0 && <p className="empty">No committed keys yet.</p>}
            {Object.entries(cluster.kv).map(([entryKey, entryValue]) => (
              <div key={entryKey} className="kv-row">
                <code>{entryKey}</code>
                <span>{entryValue}</span>
              </div>
            ))}
          </div>
        </section>
      </section>
    </main>
  );
}

function StatusPill({ online }: { online: boolean }) {
  return (
    <div className={`status-pill ${online ? "online" : "offline"}`}>
      <span />
      {online ? "Control plane online" : "Control plane offline"}
    </div>
  );
}

function ClusterMap({
  cluster,
  selectedNode,
  onSelect
}: {
  cluster: ClusterSnapshot;
  selectedNode: string;
  onSelect: (nodeId: string) => void;
}) {
  const positions = [
    { x: 50, y: 18 },
    { x: 20, y: 72 },
    { x: 80, y: 72 }
  ];

  return (
    <div className="cluster-map">
      <svg viewBox="0 0 100 100" role="img" aria-label="Raft cluster replication links">
        <defs>
          <filter id="glow">
            <feGaussianBlur stdDeviation="1.8" result="blur" />
            <feMerge>
              <feMergeNode in="blur" />
              <feMergeNode in="SourceGraphic" />
            </feMerge>
          </filter>
        </defs>
        {positions.slice(1).map((position, index) => (
          <line
            key={`${position.x}-${position.y}`}
            x1={positions[0].x}
            y1={positions[0].y}
            x2={position.x}
            y2={position.y}
            className={`replication-link ${cluster.nodes[index + 1]?.available ? "active" : "muted"}`}
          />
        ))}
      </svg>

      {cluster.nodes.map((node, index) => {
        const position = positions[index] ?? positions[0];
        return (
          <button
            key={node.nodeId}
            className={`node-orb ${node.role.toLowerCase()} ${node.available ? "" : "down"} ${selectedNode === node.nodeId ? "selected" : ""}`}
            style={{ left: `${position.x}%`, top: `${position.y}%` }}
            onClick={() => onSelect(node.nodeId)}
          >
            <span className="orb-core" />
            <strong>{node.nodeId}</strong>
            <small>{node.role}</small>
          </button>
        );
      })}
    </div>
  );
}

function EventRow({ event }: { event: ClusterEvent }) {
  return (
    <div className="event-row">
      <span className="event-sequence">#{event.sequence}</span>
      <span className="event-type">{event.type}</span>
      <span className="event-node">{event.nodeId}</span>
      <p>{event.message}</p>
    </div>
  );
}

function Metric({ label, value }: { label: string; value: string | number }) {
  return (
    <div className="metric">
      <dt>{label}</dt>
      <dd>{value}</dd>
    </div>
  );
}

createRoot(document.getElementById("root")!).render(
  <React.StrictMode>
    <App />
  </React.StrictMode>
);
