import json
import threading
import unittest
from http.client import HTTPConnection

from raftkv_control_plane.http_app import create_server


class ControlPlaneHTTPTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.server, _ = create_server("127.0.0.1", 0)
        cls.port = cls.server.server_address[1]
        cls.thread = threading.Thread(target=cls.server.serve_forever)
        cls.thread.daemon = True
        cls.thread.start()

    @classmethod
    def tearDownClass(cls):
        cls.server.shutdown()
        cls.thread.join(timeout=2)

    def request(self, method, path, body=None):
        connection = HTTPConnection("127.0.0.1", self.port, timeout=5)
        payload = None
        headers = {}
        if body is not None:
            payload = json.dumps(body)
            headers["Content-Type"] = "application/json"
        connection.request(method, path, body=payload, headers=headers)
        response = connection.getresponse()
        data = json.loads(response.read().decode("utf-8"))
        connection.close()
        return response.status, data

    def setUp(self):
        self.request("POST", "/api/v1/demos/reset", {})

    def test_health(self):
        status, data = self.request("GET", "/health")
        self.assertEqual(status, 200)
        self.assertEqual(data["status"], "ok")

    def test_cluster_status_has_leader_and_three_nodes(self):
        status, data = self.request("GET", "/api/v1/cluster")
        self.assertEqual(status, 200)
        self.assertEqual(data["leaderId"], "node1")
        self.assertEqual(data["majority"], 2)
        self.assertEqual(len(data["nodes"]), 3)

    def test_put_get_delete_command_flow(self):
        status, put = self.request(
            "POST",
            "/api/v1/commands",
            {"command": "PUT", "key": "user:1", "value": "Abhishek"},
        )
        self.assertEqual(status, 200)
        self.assertTrue(put["committed"])
        self.assertEqual(put["acknowledgements"], 3)

        status, get = self.request(
            "POST", "/api/v1/commands", {"command": "GET", "key": "user:1"}
        )
        self.assertEqual(status, 200)
        self.assertEqual(get["value"], "Abhishek")

        status, delete = self.request(
            "POST", "/api/v1/commands", {"command": "DELETE", "key": "user:1"}
        )
        self.assertEqual(status, 200)
        self.assertTrue(delete["committed"])

        status, missing = self.request(
            "POST", "/api/v1/commands", {"command": "GET", "key": "user:1"}
        )
        self.assertEqual(status, 409)
        self.assertEqual(missing["message"], "not found")

    def test_no_quorum_rejects_write(self):
        self.request("POST", "/api/v1/faults/node", {"nodeId": "node2", "available": False})
        self.request("POST", "/api/v1/faults/node", {"nodeId": "node3", "available": False})

        status, result = self.request(
            "POST",
            "/api/v1/commands",
            {"command": "PUT", "key": "payment:55", "value": "success"},
        )

        self.assertEqual(status, 409)
        self.assertFalse(result["committed"])
        self.assertEqual(result["acknowledgements"], 1)
        self.assertEqual(result["majority"], 2)

    def test_events_are_recorded(self):
        self.request(
            "POST",
            "/api/v1/commands",
            {"command": "PUT", "key": "event:test", "value": "ok"},
        )

        status, data = self.request("GET", "/api/v1/events")
        self.assertEqual(status, 200)
        event_types = [event["type"] for event in data["events"]]
        self.assertIn("BECAME_LEADER", event_types)
        self.assertIn("LOG_APPENDED", event_types)
        self.assertIn("ENTRY_COMMITTED", event_types)
        self.assertIn("ENTRY_APPLIED", event_types)


if __name__ == "__main__":
    unittest.main()
