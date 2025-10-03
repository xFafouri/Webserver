#!/usr/bin/env python3

import cgitb
import os
import sys
import urllib.parse

cgitb.enable()

UPLOAD_DIR = "www/main/uploads"  # Folder where files are stored

def generate_html_page(title, body_content, alert_message=None):
    alert_html = f"<script>alert('{alert_message}');</script>" if alert_message else ""
    return f"""
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <title>{title}</title>
        <style>
            body {{
                font-family: Arial, sans-serif;
                background: #f0f0f0;
                display:flex;
                justify-content:center;
                align-items:flex-start;
                padding-top:50px;
            }}
            .container {{
                background:#fff;
                padding:2rem;
                border-radius:12px;
                box-shadow:0 4px 12px rgba(0,0,0,0.1);
                width:450px;
            }}
            h2 {{ text-align:center; margin-bottom:1rem; }}
            ul {{ list-style:none; padding-left:0; }}
            li {{ margin:8px 0; }}
            a {{ text-decoration:none; color:#007BFF; }}
            a:hover {{ text-decoration:underline; }}
        </style>
    </head>
    <body>
        <div class="container">
            {body_content}
        </div>
        {alert_html}
    </body>
    </html>
    """

def list_files():
    """Return HTML list of files with clean URIs"""
    try:
        files = os.listdir(UPLOAD_DIR)
    except PermissionError:
        return "", "⚠️ Accès interdit au dossier uploads (403 Forbidden)."
    except FileNotFoundError:
        return "", "⚠️ Dossier uploads introuvable (404 Not Found)."
    except Exception as e:
        return "", f"⚠️ Erreur inattendue: {e}"

    if not files:
        return "<p>Aucun fichier disponible.</p>", None

    html_list = "<h2>Fichiers disponibles :</h2><ul>"
    for filename in files:
        if filename.startswith("."):  # skip hidden files
            continue
        safe_name = urllib.parse.quote(filename)
        # The link is now /uploads/filename
        html_list += f'<li><a href="/uploads/{safe_name}">{filename}</a></li>'
    html_list += "</ul>"
    return html_list, None

def serve_file(path_info):
    """Serve a file for download based on PATH_INFO after /uploads/"""
    resource = path_info.lstrip("/")  # Remove leading slash
    resource = urllib.parse.unquote(resource)
    file_path = os.path.join(UPLOAD_DIR, resource)

    if not os.path.isfile(file_path):
        print("Content-Type: text/html; charset=utf-8")
        print()
        print(generate_html_page("Erreur", "", f"⚠️ Le fichier '{resource}' n'existe pas (404 Not Found)."))
        return

    try:
        print(f"Content-Type: application/octet-stream")
        print(f"Content-Disposition: attachment; filename=\"{resource}\"")
        print(f"Content-Length: {os.path.getsize(file_path)}")
        print()  # End headers
        sys.stdout.flush()

        with open(file_path, "rb") as f:
            while chunk := f.read(8192):
                sys.stdout.buffer.write(chunk)
                sys.stdout.buffer.flush()
    except PermissionError:
        print("Content-Type: text/html; charset=utf-8")
        print()
        print(generate_html_page("Erreur", "", f"⚠️ Accès refusé au fichier '{resource}' (403 Forbidden)."))
    except Exception as e:
        print("Content-Type: text/html; charset=utf-8")
        print()
        print(generate_html_page("Erreur", "", f"⚠️ Erreur lors de l'accès au fichier '{resource}': {e}"))

def main():
    # The webserver should route /uploads/* to this CGI
    path_info = os.environ.get("PATH_INFO", "").lstrip("/")

    if path_info:
        serve_file(path_info)
    else:
        html_content, alert = list_files()
        print("Content-Type: text/html; charset=utf-8")
        print()
        print(generate_html_page("Liste des fichiers", html_content, alert))

if __name__ == "__main__":
    main()
