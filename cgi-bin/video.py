#!/usr/bin/env python3
import sys
import os

file_path = "/home/hfafouri/Desktop/webserver20/cgi-bin/video.mp4"

# Get exact file size
file_size = os.path.getsize(file_path)

sys.stdout.write(f"Content-Type: video/mp4\r\n")
sys.stdout.write(f"Content-Length: {file_size}\r\n")
sys.stdout.write("\r\n")  # End of headers

with open(file_path, "rb") as f:
    sys.stdout.buffer.write(f.read())
