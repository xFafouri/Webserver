import socket
import time

body_chunks = ["Hello", ", worl", "d"]

request_headers = (
    "POST / HTTP/1.1\r\n"
    "Host: localhost:8081\r\n"
    "User-Agent: PythonClient/1.0\r\n"
    "Transfer-Encoding: chunked\r\n"
    "Content-Type: text/plain\r\n"
    "\r\n"
)

def send_chunked_request():
    s = socket.socket()
    s.connect(("localhost", 8081))
    s.sendall(request_headers.encode())

    for chunk in body_chunks:
        size = hex(len(chunk))[2:]
        s.sendall(f"{size}\r\n{chunk}\r\n".encode())
        time.sleep(0.5)  # simulate delay between chunks

    s.sendall(b"0\r\n\r\n")  # End of chunks
    response = s.recv(4096)
    print("Response:\n", response.decode())
    s.close()

send_chunked_request()

