#!/usr/bin/env python3

import cgi
import cgitb
cgitb.enable()  # Enable debugging

print("Content-Type: text/html\n")  # Important: must print HTTP headers followed by blank line

form = cgi.FieldStorage()

print("<html><head><title>CGI Test</title></head><body>")

# Handle GET query parameters
if "name" in form:
    name = form.getvalue("name")
    print(f"<h2>Hello, {cgi.escape(name)}!</h2>")

# Handle file upload (POST multipart/form-data)
if "video" in form:
    video_item = form["video"]
    if video_item.filename:
        # Save uploaded file (optional)
        with open("/tmp/uploaded_video.mp4", "wb") as f:
            f.write(video_item.file.read())
        print(f"<p>Uploaded video filename: {cgi.escape(video_item.filename)}</p>")
    else:
        print("<p>No video file uploaded.</p>")
else:
    print("<p>No video data found in form.</p>")

print("</body></html>")
