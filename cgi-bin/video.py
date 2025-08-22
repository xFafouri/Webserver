#!/usr/bin/env python3
import sys
import os

# Path to the video file you want to serve
file_path = "/home/hfafouri/Desktop/webserver20/cgi-bin/video.mp4"

try:
    # Get size of the file
    file_size = os.path.getsize(file_path)

    # Write headers
    sys.stdout.write("Content-Type: video/mp4\r\n")
    sys.stdout.write(f"Content-Length: {file_size}\r\n")
    sys.stdout.write("\r\n")  # End of headers

    # Stream the video binary
    with open(file_path, "rb") as f:
        sys.stdout.buffer.write(f.read())

except Exception as e:
    # In case of error, return a plain text error message
    error_message = f"Error: {str(e)}\n"
    sys.stdout.write("Content-Type: text/plain\r\n")
    sys.stdout.write(f"Content-Length: {len(error_message)}\r\n")
    sys.stdout.write("\r\n")
    sys.stdout.write(error_message)
