#!/usr/bin/env python3

print("Content-Type: video/mp4")
print("Content-Length: 583023")  # exact size of the file in bytes
print()  # this blank line is crucial

with open("/home/hfafouri/Desktop/webserver/cgi-bin/video_short.mp4", "rb") as f:
    ie
    sys.stdout.buffer.write(f.read())