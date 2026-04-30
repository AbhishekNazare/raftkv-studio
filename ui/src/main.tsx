import React, { useEffect, useMemo, useState } from "react";
import { createRoot } from "react-dom/client";
import {
  getCluster,
  getEvents,
  resetDemo,
  sendCommand,
  setNodeAvailability
} from "./api";
import type { ClusterEvent, ClusterSnapshot, CommandResult, NodeStatus } from "./types";
import "./styles.css";

const emptyCluster: ClusterSnapshot = {
  leaderId: null,
  majority: 2,
  nodes: [],
  kv: {}
};

function App() {
  const [cluster, setCluster] = useState<ClusterSnapshot>(emptyCluster);
  const [events, setEvents] = useState<ClusterEvent[]>([]);
  const [apiOnline, setApiOnline] = useState(false);
  const [selectedNode, setSelectedNode] = useState<string>("node1");
  const [command, setCommand] = useState("PUT");
  const [key, setKey] = useState("user:1");
  const [value, setValue] = useState("Abhishek");
  const [lastResult, setLastResult] = useState<CommandResult | null>(null);

  async function refresh() {
    try {
      const [clusterData, eventData] = await Promise.all([getCluster(), getEvents()]);
      setCluster(clusterData);
      setEvents(eventData);
      setApiOnline(true);
      if (!clusterData.nodes.some((node) => node.nodeId === selectedNode)) {
        setSelectedNode(clusterData.nodes[0]?.nodeId ?? "node1");
      }
    } catch {
      setApiOnline(false);
    }
  }

  useEffect(() => {
    refresh();
    const timer = window.setInterval(refresh, 2000);
    return () => window.clearInterval(timer);
  }, []);

  const selected = useMemo(
    () => cluster.nodes.find((node) => node.nodeId === selectedNode) ?? cluster.nodes[0],
    [cluster.nodes, selectedNode]
  );

  async function runCommand() {
    const result = await sendCommand(command, key, command === "PUT" ? value : undefined);
    setLastResult(result);
    await refresh();
  }

  async function toggleNode(node: NodeStatus) {
    await setNodeAvailability(node.nodeId, !node.available);
    await refresh();
  }

  async function reset() {
    await resetDemo();
    setLastResult(null);
    await refresh();
  }

  return (
    <main className="app-shell">
      <aside className="sidebar">
        <div className="brand">
          <span className="brand-mark">RK</span>
          <div>
            <h1>RaftKV Studio</h1>
            <p>Consensus control room</p>
          </div>
        </div>
        <nav className="nav-list" aria-label="Dashboard sections">
          <a className="active">Dashboard</a>
          <a>Commands</a>
          <a>Events</a>
          <a>Faults</a>
          <a>State</a>
        </nav>
        <div className="api-status">
          <span className={apiOnline ? "dot online" : "dot offline"} />
          <span>{apiOnline ? "Control plane online" : "Control plane offline"}</span>
        </div>
      </aside>

      <section className="workspace">
        <header className="topbar">
          <div>
            <p className="eyebrow">3-node Raft cluster</p>
            <h2>{cluster.leaderId ? `${cluster.leaderId} is leader` : "No leader available"}</h2>
          </div>
          <div className="topbar-actions">
            <button onClick={refresh}>Refresh</button>
            <button className="primary" onClick={reset}>Reset Demo</button>
          </div>
        </header>

        <section className="dashboard-grid">
          <div className="panel topology-panel">
            <div className="panel-header">
              <h3>Cluster Topology</h3>
              <span>majority {cluster.majority}</span>
            </div>
            <div className="topology">
              {cluster.nodes.map((node) => (
                <button
                  key={node.nodeId}
                  className={`node-tile ${node.role.toLowerCase()} ${node.available ? "" : "down"} ${
                    selected?.nodeId === node.nodeId ? "selected" : ""
                  }`}
                  onClick={() => setSelectedNode(node.nodeId)}
                >
                  <span className="node-name">{node.nodeId}</span>
                  <span className="node-role">{node.role}</span>
                  <span className="node-term">term {node.term}</span>
                </button>
              ))}
            </div>
          </div>

          <div className="panel node-panel">
            <div className="panel-header">
              <h3>Node Details</h3>
              {selected && <span>{selected.available ? "available" : "unavailable"}</span>}
            </div>
            {selected ? (
              <dl className="metrics">
                <Metric label="Role" value={selected.role} />
                <Metric label="Term" value={selected.term} />
                <Metric label="Commit Index" value={selected.commitIndex} />
                <Metric label="Last Applied" value={selected.lastApplied} />
                <Metric label="Last Log Index" value={selected.lastLogIndex} />
              </dl>
            ) : (
              <p className="empty">Start the control plane to load nodes.</p>
            )}
          </div>

          <div className="panel command-panel">
            <div className="panel-header">
              <h3>Command Terminal</h3>
              <span>HTTP API</span>
            </div>
            <div className="terminal-row">
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
              <button className="primary" onClick={runCommand} disabled={!apiOnline}>Run</button>
            </div>
            <div className="terminal-output">
              <span>raftkv&gt; {command.toLowerCase()} {key}{command === "PUT" ? ` ${value}` : ""}</span>
              <strong>{lastResult ? lastResult.message : "waiting for command"}</strong>
              {lastResult && (
                <span>
                  ack {lastResult.acknowledgements}/{cluster.nodes.length || 3}, committed{" "}
                  {String(lastResult.committed)}
                </span>
              )}
            </div>
          </div>

          <div className="panel fault-panel">
            <div className="panel-header">
              <h3>Fault Controls</h3>
              <span>node availability</span>
            </div>
            <div className="fault-list">
              {cluster.nodes.map((node) => (
                <button key={node.nodeId} onClick={() => toggleNode(node)} disabled={!apiOnline}>
                  {node.available ? "Stop" : "Start"} {node.nodeId}
                </button>
              ))}
            </div>
          </div>

          <div className="panel event-panel">
            <div className="panel-header">
              <h3>Event Timeline</h3>
              <span>{events.length} events</span>
            </div>
            <div className="event-list">
              {events.slice().reverse().slice(0, 12).map((event) => (
                <div key={event.sequence} className="event-row">
                  <span className="event-type">{event.type}</span>
                  <span>{event.nodeId}</span>
                  <p>{event.message}</p>
                </div>
              ))}
            </div>
          </div>

          <div className="panel state-panel">
            <div className="panel-header">
              <h3>KV State</h3>
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
          </div>
        </section>
      </section>
    </main>
  );
}

function Metric({ label, value }: { label: string; value: string | number }) {
  return (
    <div>
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

