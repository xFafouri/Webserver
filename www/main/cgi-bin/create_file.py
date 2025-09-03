#!/usr/bin/env python3
import socket
import time

HOST = "127.0.0.1"
PORT = 3434
TIMEOUT = 5   # seconds

def run_server():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.bind((HOST, PORT))
        server.listen(5)
        print(f"Server listening on {HOST}:{PORT}")

        while True:
            conn, addr = server.accept()
            print(f"Connection from {addr}")

            # Apply timeout on this connection
            conn.settimeout(TIMEOUT)

            try:
                data = conn.recv(1024)
                if not data:
                    print("Client closed connection")
                else:
                    print("Received:", data.decode(errors="ignore"))
                    conn.sendall(b"HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nHello")
            except socket.timeout:
                print(f"Connection {addr} timed out after {TIMEOUT} seconds")
            except Exception as e:
                print("Error:", e)
            finally:
                conn.close()

if __name__ == "__main__":
    run_server()
