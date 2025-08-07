#!/usr/bin/env python3

import cgi

print("Content-Type: text/html")
print()

form = cgi.FieldStorage()
name = form.getvalue("name", "Guest")
lang = form.getvalue("lang", "Python")

print(f"<html><body><h1>Hello, {name}!</h1><p>Your favorite language is {lang}.</p></body></html>")
