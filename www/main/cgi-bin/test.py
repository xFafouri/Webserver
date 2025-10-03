#!/usr/bin/env python3
print("Content-Type: text/html\n")
import cgi
import cgitb
cgitb.enable()  # Enable debugging

form = cgi.FieldStorage()
name = form.getvalue("name", "Guest")
email = form.getvalue("email", "")
message = form.getvalue("message", "")
method = form.getvalue("method", "GET")

html = f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>CGI Test Site</title>
    <style>
        body {{
            font-family: Arial, sans-serif;
            background: #f7f7f7;
            margin: 0;
            padding: 0;
        }}
        header {{
            background: #4CAF50;
            color: white;
            padding: 1em;
            text-align: center;
        }}
        main {{
            max-width: 800px;
            margin: auto;
            background: white;
            padding: 2em;
            box-shadow: 0 0 10px rgba(0,0,0,0.1);
        }}
        form {{
            display: flex;
            flex-direction: column;
        }}
        input, textarea {{
            padding: 10px;
            margin: 10px 0;
            border: 1px solid #ccc;
        }}
        input[type="submit"] {{
            background: #4CAF50;
            color: white;
            cursor: pointer;
            border: none;
        }}
        .response {{
            background: #e7f4e4;
            padding: 1em;
            margin-top: 2em;
            border: 1px solid #c3e6cb;
        }}
    </style>
</head>
<body>
    <header>
        <h1>Welcome to My CGI Test Page</h1>
    </header>
    <main>
        <p>This is a test page to validate GET and POST requests using CGI in Python.</p>

        <h2>Send a Message</h2>
        <form method="POST" action="/cgi-bin/test.py">
            <input type="text" name="name" placeholder="Your Name" required>
            <input type="email" name="email" placeholder="Your Email" required>
            <textarea name="message" placeholder="Your Message" rows="5" required></textarea>
            <input type="submit" value="Submit">
        </form>
"""

if "name" in form and "message" in form:
    html += f"""
        <div class="response">
            <h3>Thank you, {name}!</h3>
            <p><strong>Email:</strong> {email}</p>
            <p><strong>Message:</strong><br>{message}</p>
        </div>
    """

html += """
    </main>
</body>
</html>
"""

print(html)
