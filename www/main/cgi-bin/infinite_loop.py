#!/usr/bin/env python3

import sys
import time

# Standard CGI header
print("Content-Type: text/plain\r\n\r\n")
print("Starting infinite loop... (this should hang without timeout)")

# Infinite loop
while True:
    time.sleep(1)  # prevent 100% CPU usage
    print("Still looping...", flush=True)
