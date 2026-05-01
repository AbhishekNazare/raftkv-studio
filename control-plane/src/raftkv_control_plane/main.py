import argparse

from .http_app import create_server


def main() -> None:
    parser = argparse.ArgumentParser(description="Run the RaftKV control plane")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8090)
    args = parser.parse_args()

    server, _ = create_server(args.host, args.port)
    print(f"raftkv-control-plane listening on http://{args.host}:{args.port}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nraftkv-control-plane stopped")
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
