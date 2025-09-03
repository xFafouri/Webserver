#!/usr/bin/env python3

import sys
import time

# Standard CGI header
print("Content-Type: text/plain\r\n\r\n")
print("Starting infinite loop... ")

# Infinite loop
while True:
    print("Still looping...", flush=True)
    time.sleep(5)  # prevent 100% CPU usage
