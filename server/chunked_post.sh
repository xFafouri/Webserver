#!/bin/bash
(echo -en "POST /upload HTTP/1.1\r\nHost: localhost:8888\r\nTransfer-Encoding: chunked\r\nContent-Type: text/plain\r\n\r\n"; \
sleep 1; echo -en "5\r\nhamza\r\n"; \
sleep 1; echo -en "6\r\n fafouri\r\n"; \
sleep 1; echo -en "0\r\n\r\n") | nc localhost 8888

