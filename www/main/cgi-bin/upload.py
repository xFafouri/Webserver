#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import cgi
import os
import cgitb

# Enable detailed error reports
cgitb.enable()

upload_dir = "./www/main/cgi-bin/uploads/"
os.makedirs(upload_dir, exist_ok=True)

# Parse form data
form = cgi.FieldStorage()

print("Content-Type: text/html; charset=utf-8")
print()

value = ""

if "file" in form:
    file_item = form["file"]

    if file_item.filename:
        filename = os.path.basename(file_item.filename)
        filepath = os.path.join(upload_dir, filename)

        try:
            with open(filepath, "wb") as f:
                while True:
                    chunk = file_item.file.read(1024)
                    if not chunk:
                        break
                    f.write(chunk)

            value = f"✅ Le fichier <b>{filename}</b> a été uploadé avec succès.<br>📂 Enregistré à : <code>{filepath}</code>"
        except Exception as e:
            value = f"❌ Erreur lors de l'enregistrement du fichier : {e}"
    else:
        value = "⚠️ Aucun fichier n'a été sélectionné."
else:
    value = "⚠️ Aucun champ 'file' trouvé dans le formulaire."

# Return HTML page
html_content = f"""
<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <title>Résultat Upload</title>
    <style>
        body {{
            font-family: 'Inter', sans-serif;
            background-color: #f0f0f0;
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
            color: #333;
        }}
        .container {{
            text-align: center;
            background: #fff;
            padding: 2rem;
            border-radius: 12px;
            box-shadow: 0 4px 8px rgba(0,0,0,0.1);
        }}
        a {{
            display: inline-block;
            margin-top: 1rem;
            text-decoration: none;
            color: #007BFF;
        }}
    </style>
</head>
<body>
    <div class="container">
        <h2>Résultat de l'upload</h2>
        <p>{value}</p>
        <a href="/">⬅ Retour à l'accueil</a>
    </div>
</body>
</html>
"""

print(html_content)
