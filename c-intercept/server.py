#!/usr/bin/env python3

import argparse
import socket
from datetime import datetime


DEFAULT_HOST = "172.44.0.1"
DEFAULT_PORT = 9000


def parse_args():
    parser = argparse.ArgumentParser(
        description="TCP listener for Unikraft intercept payloads."
    )
    parser.add_argument("--host", default=DEFAULT_HOST, help="Bind address")
    parser.add_argument("--port", default=DEFAULT_PORT, type=int, help="Bind port")
    parser.add_argument(
        "--raw",
        action="store_true",
        help="Print repr(data) instead of decoded text",
    )
    return parser.parse_args()


def timestamp():
    return datetime.now().strftime("%H:%M:%S")


def format_payload(data, raw):
    if raw:
        return repr(data)

    try:
        return data.decode("utf-8")
    except UnicodeDecodeError:
        return f"{data!r} [hex={data.hex()}]"


def serve(host, port, raw):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.bind((host, port))
        server.listen()
        print(f"[{timestamp()}] listening on {host}:{port}")

        conn_id = 0
        while True:
            conn, addr = server.accept()
            conn_id += 1
            print(f"[{timestamp()}] conn#{conn_id} accepted from {addr[0]}:{addr[1]}")

            with conn:
                total_bytes = 0

                while True:
                    data = conn.recv(4096)
                    if not data:
                        print(
                            f"[{timestamp()}] conn#{conn_id} closed after {total_bytes} bytes"
                        )
                        break

                    total_bytes += len(data)
                    payload = format_payload(data, raw)
                    print(f"[{timestamp()}] conn#{conn_id} recv {len(data)}B: {payload}")


def main():
    args = parse_args()
    serve(args.host, args.port, args.raw)


if __name__ == "__main__":
    main()
