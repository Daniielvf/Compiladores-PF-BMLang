import json
import subprocess
from pathlib import Path
from http.server import HTTPServer, SimpleHTTPRequestHandler

# carpeta del proyecto
BASE = Path(__file__).resolve().parent


class Servidor(SimpleHTTPRequestHandler):
    # mostrar los archivos de la interfaz
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(BASE / "Web"), **kwargs)

    # responder al editor
    def responder(self, estado, datos):
        self.send_response(estado)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(json.dumps(datos).encode())

    # recibir y analizar el programa
    def do_POST(self):
        try:
            if self.path != "/api/procesar":
                raise ValueError("Ruta desconocida")
            cantidad = int(self.headers.get("Content-Length", 0))
            if not 0 < cantidad <= 65536:
                raise ValueError("Solicitud vacía o demasiado grande")
            datos = json.loads(self.rfile.read(cantidad))
            if datos.get("accion") != "intermedio":
                raise ValueError("La base todavía no genera NASM ni ejecutables")
            codigo = datos.get("codigo", "")
            if not isinstance(codigo, str) or not codigo.strip():
                raise ValueError("Escribe un programa primero")

            # construir el compilador
            comandos = [
                ["bison", "-d", "analizador.y"],
                ["flex", "analizador.l"],
                ["gcc", "analizador.tab.c", "lex.yy.c", "-o", "analizador", "-lfl"]
            ]
            for comando in comandos:
                subprocess.run(comando, cwd=BASE, capture_output=True,
                               text=True, check=True, timeout=30)

            # ejecutar el analizador
            proceso = subprocess.run(
                [str(BASE / "analizador")], input=codigo, cwd=BASE,
                capture_output=True, text=True, check=True, timeout=5
            )
            texto = proceso.stdout
            correcto = ("Analisis sintactico correcto" in texto
                        and "Error sintactico" not in texto)

            # devolver el resultado
            self.responder(200 if correcto else 422, {
                "ok": correcto,
                "mensaje": "Análisis correcto" if correcto else "Error de sintaxis",
                "codigo": texto.replace("Analisis sintactico correcto", "").strip()
                          if correcto else "",
                "detalle": "Intermedio parcial de la base actual"
                           if correcto else texto + proceso.stderr
            })
        except Exception as error:
            self.responder(400, {
                "ok": False,
                "mensaje": "No se pudo completar la solicitud",
                "detalle": getattr(error, "stderr", None) or str(error)
            })


# iniciar el servidor
servidor = HTTPServer(("127.0.0.1", 8000), Servidor)
print("Abre http://127.0.0.1:8000")

try:
    servidor.serve_forever()
except KeyboardInterrupt:
    print("\nServidor detenido")
finally:
    servidor.server_close()