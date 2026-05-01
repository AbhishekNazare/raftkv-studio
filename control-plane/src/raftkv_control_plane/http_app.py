import json
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from typing import Any, Callable, Tuple
from urllib.parse import urlparse

from .cluster_service import ClusterService


class ControlPlaneApp:
    def __init__(self, service: ClusterService) -> None:
        self.service = service

    def make_handler(self) -> Callable[..., BaseHTTPRequestHandler]:
        service = self.service

        class Handler(BaseHTTPRequestHandler):
            def do_GET(self) -> None:
                path = urlparse(self.path).path
                if path == "/health":
                    self._write_json(200, {"status": "ok"})
                    return
                if path == "/api/v1/cluster":
                    self._write_json(200, service.snapshot().to_dict())
                    return
                if path == "/api/v1/events":
                    self._write_json(200, {"events": [event.to_dict() for event in service.events()]})
                    return
                self._write_json(404, {"error": "not found"})

            def do_POST(self) -> None:
                path = urlparse(self.path).path
                body = self._read_json()

                try:
                    if path == "/api/v1/commands":
                        result = service.run_command(
                            str(body.get("command", "")),
                            str(body.get("key", "")),
                            body.get("value"),
                        )
                        status = 200 if result.ok else 409
                        self._write_json(status, result.to_dict())
                        return

                    if path == "/api/v1/faults/node":
                        snapshot = service.set_node_available(
                            str(body.get("nodeId", "")),
                            bool(body.get("available", True)),
                        )
                        self._write_json(200, snapshot.to_dict())
                        return

                    if path == "/api/v1/faults/partition":
                        snapshot = service.partition_node(str(body.get("nodeId", "")))
                        self._write_json(200, snapshot.to_dict())
                        return

                    if path == "/api/v1/faults/heal":
                        snapshot = service.heal_node(str(body.get("nodeId", "")))
                        self._write_json(200, snapshot.to_dict())
                        return

                    if path == "/api/v1/snapshots/create":
                        snapshot = service.create_snapshot()
                        self._write_json(200, snapshot.to_dict())
                        return

                    if path == "/api/v1/demos/reset":
                        self._write_json(200, service.reset().to_dict())
                        return

                    if path == "/api/v1/demos/run":
                        result = service.run_demo(str(body.get("scenario", "")))
                        self._write_json(200, result)
                        return

                    self._write_json(404, {"error": "not found"})
                except (KeyError, ValueError) as exc:
                    self._write_json(400, {"error": str(exc)})
                except RuntimeError as exc:
                    self._write_json(503, {"error": str(exc)})

            def log_message(self, format: str, *args: Any) -> None:
                return

            def _read_json(self) -> dict:
                length = int(self.headers.get("Content-Length", "0"))
                if length == 0:
                    return {}
                raw = self.rfile.read(length).decode("utf-8")
                return json.loads(raw)

            def _write_json(self, status: int, body: dict) -> None:
                payload = json.dumps(body, sort_keys=True).encode("utf-8")
                self.send_response(status)
                self.send_header("Content-Type", "application/json")
                self.send_header("Content-Length", str(len(payload)))
                self.send_header("Access-Control-Allow-Origin", "*")
                self.end_headers()
                self.wfile.write(payload)

        return Handler


def create_server(host: str, port: int) -> Tuple[ThreadingHTTPServer, ClusterService]:
    service = ClusterService()
    app = ControlPlaneApp(service)
    return ThreadingHTTPServer((host, port), app.make_handler()), service
