# Webserv

Webserv is a lightweight HTTP server written in C++98, created for the **42 Network** curriculum.  
It supports essential features like:

- HTTP/1.1 GET, POST, and DELETE methods
- Custom server configuration files
- Handling of multiple clients via `poll()`
- CGI execution (e.g. Python or PHP scripts)
- Autoindex, error pages, file uploads, and more

This project was built from scratch to better understand low-level networking, HTTP protocols, and server design.

> 🔧 **Language:** C++98
> 📡 **Sockets API:** `poll()`  
> 📁 **Standard:** RFC 2616 (HTTP/1.1)
