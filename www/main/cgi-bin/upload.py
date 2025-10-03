# #!/usr/bin/env python3
# # -*- coding: utf-8 -*-

# import cgi
# import os
# import cgitb

# # Enable detailed error reports
# cgitb.enable()

# upload_dir = "www/main/uploads"
# os.makedirs(upload_dir, exist_ok=True)

# # Parse form data
# form = cgi.FieldStorage()

# print("Content-Type: text/html; charset=utf-8")
# print()

# value = ""

# if "file" in form:
#     file_item = form["file"]

#     if file_item.filename:
#         filename = os.path.basename(file_item.filename)
#         filepath = os.path.join(upload_dir, filename)

#         try:
#             with open(filepath, "wb") as f:
#                 while True:
#                     chunk = file_item.file.read(1024)
#                     if not chunk:
#                         break
#                     f.write(chunk)

#             value = f"✅ Le fichier <b>{filename}</b> a été uploadé avec succès.<br>📂 Enregistré à : <code>{filepath}</code>"
#         except Exception as e:
#             value = f"❌ Erreur lors de l'enregistrement du fichier : {e}"
#     else:
#         value = "⚠️ Aucun fichier n'a été sélectionné."
# else:
#     value = "⚠️ Aucun champ 'file' trouvé dans le formulaire."

# # Return HTML page
# html_content = f"""
# <!DOCTYPE html>
# <html lang="fr">
# <head>
#     <meta charset="UTF-8">
#     <title>Résultat Upload</title>
#     <style>
#         body {{
#             font-family: 'Inter', sans-serif;
#             background-color: #f0f0f0;
#             display: flex;
#             justify-content: center;
#             align-items: center;
#             height: 100vh;
#             color: #333;
#         }}
#         .container {{
#             text-align: center;
#             background: #fff;
#             padding: 2rem;
#             border-radius: 12px;
#             box-shadow: 0 4px 8px rgba(0,0,0,0.1);
#         }}
#         a {{
#             display: inline-block;
#             margin-top: 1rem;
#             text-decoration: none;
#             color: #007BFF;
#         }}
#     </style>
# </head>
# <body>
#     <div class="container">
#         <h2>Résultat de l'upload</h2>
#         <p>{value}</p>
#         <a href="/">⬅ Retour à l'accueil</a>
#     </div>
# </body>
# </html>
# """

# print(html_content)


#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import cgi
import os
import cgitb

cgitb.enable()  # Detailed error reporting in browser console (optional)

# Helper function to return HTML page with a JS alert
def show_alert(message, title="Upload Result"):
    print("Content-Type: text/html; charset=utf-8")
    print()
    print(f"""
    <!DOCTYPE html>
    <html lang="fr">
    <head>
        <meta charset="UTF-8">
        <title>{title}</title>
        <script>
            alert("{message}");
        </script>
    </head>
    <body>
        <h2>{title}</h2>
        <p>{message}</p>
        <a href='/'>⬅ Retour à l'accueil</a>
    </body>
    </html>
    """)
    exit(0)

try:
    # Only allow POST requests
    if os.environ.get("REQUEST_METHOD") != "POST":
        show_alert("⚠️ Méthode non autorisée ! Seules les requêtes POST sont acceptées.", "405 Method Not Allowed")

    # Upload directory relative to CGI script
    script_dir = os.path.dirname(os.path.abspath(__file__))
    upload_dir = os.path.abspath(os.path.join(script_dir, "..", "uploads"))

    # Ensure upload directory exists
    try:
        os.makedirs(upload_dir, exist_ok=True)
    except PermissionError:
        show_alert(f"❌ Permission refusée pour créer le dossier d'uploads : {upload_dir}", "403 Forbidden")

    # Parse form data
    form = cgi.FieldStorage()
    if "file" not in form:
        show_alert("⚠️ Aucun fichier sélectionné ou champ 'file' manquant.", "400 Bad Request")

    file_item = form["file"]
    if not file_item.filename:
        show_alert("⚠️ Aucun fichier n'a été sélectionné.", "400 Bad Request")

    # Save the uploaded file
    filename = os.path.basename(file_item.filename)
    filepath = os.path.join(upload_dir, filename)

    try:
        with open(filepath, "wb") as f:
            while True:
                chunk = file_item.file.read(1024)
                if not chunk:
                    break
                f.write(chunk)
        # Success alert
        show_alert(f"✅ Le fichier '{filename}' a été uploadé avec succès à : {filepath}", "Upload Réussi")

    except PermissionError:
        show_alert(f"❌ Permission refusée pour écrire dans : {upload_dir}", "403 Forbidden")
    except Exception as e:
        show_alert(f"❌ Erreur lors de l'enregistrement du fichier : {e}", "500 Internal Server Error")

except Exception as e:
    # Unexpected errors
    show_alert(f"⚠️ Une erreur inattendue est survenue : {e}", "500 Internal Server Error")
