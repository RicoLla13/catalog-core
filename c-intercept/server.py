import socket

HOST = "172.44.0.1"
PORT = 9000

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind((HOST, PORT))
    s.listen()
    print(f"listening on {HOST}:{PORT}")

    while True:
        conn, addr = s.accept()
        print("accepted", addr)
        with conn:
            while True:
                data = conn.recv(4096)
                if not data:
                    break
                print("recv:", data)
