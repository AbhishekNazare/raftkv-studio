import argparse

from .http_app import create_server


def main() -> None:
    parser = argparse.ArgumentParser(description="Run the RaftKV control plane")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8090)
    args = parser.parse_args()

    server, _ = create_server(args.host, args.port)
    print(f"raftkv-control-plane listening on http://{args.host}:{args.port}")
    server.serve_forever()


if __name__ == "__main__":
    main()

