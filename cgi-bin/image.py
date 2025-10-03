#!/usr/bin/env python3

import os
import sys

# Path to your image (make sure it's a real image)
image_path = "/home/hfafouri/Desktop/webserver/cgi-bin/haifa.jpg"

try:
    with open(image_path, "rb") as f:
        image_data = f.read()

    # Output headers
    sys.stdout.buffer.write(b"Content-Type: image/png\r\n")
    sys.stdout.buffer.write(f"Content-Length: {len(image_data)}\r\n".encode())
    sys.stdout.buffer.write(b"\r\n")

    # Output image data
    sys.stdout.buffer.write(image_data)

except Exception as e:
    sys.stdout.write("Content-Type: text/plain\r\n\r\n")
    sys.stdout.write(f"Error: {str(e)}\n")
