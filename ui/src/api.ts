import type { ClusterEvent, ClusterSnapshot, CommandResult } from "./types";

const jsonHeaders = { "Content-Type": "application/json" };

async function request<T>(path: string, init?: RequestInit): Promise<T> {
  const response = await fetch(path, init);
  const body = (await response.json()) as T;
  if (!response.ok) {
    return body;
  }
  return body;
}

export async function getCluster(): Promise<ClusterSnapshot> {
  return request<ClusterSnapshot>("/api/v1/cluster");
}

export async function getEvents(): Promise<ClusterEvent[]> {
  const body = await request<{ events: ClusterEvent[] }>("/api/v1/events");
  return body.events;
}

export async function sendCommand(
  command: string,
  key: string,
  value?: string
): Promise<CommandResult> {
  return request<CommandResult>("/api/v1/commands", {
    method: "POST",
    headers: jsonHeaders,
    body: JSON.stringify({ command, key, value })
  });
}

export async function setNodeAvailability(
  nodeId: string,
  available: boolean
): Promise<ClusterSnapshot> {
  return request<ClusterSnapshot>("/api/v1/faults/node", {
    method: "POST",
    headers: jsonHeaders,
    body: JSON.stringify({ nodeId, available })
  });
}

export async function resetDemo(): Promise<ClusterSnapshot> {
  return request<ClusterSnapshot>("/api/v1/demos/reset", {
    method: "POST",
    headers: jsonHeaders,
    body: JSON.stringify({})
  });
}

