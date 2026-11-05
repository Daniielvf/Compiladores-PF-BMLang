import json
import json
import subprocess
from pathlib import Path
from http.server import HTTPServer, SimpleHTTPRequestHandler

# Carpeta principal del proyecto
BASE = Path(__file__).resolve().parent


class Servidor(SimpleHTTPRequestHandler):
    # Mostrar los archivos del IDE web
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(BASE / "Web"), **kwargs)

    # Enviar respuestas al navegador
    def responder(self, estado, datos):
        self.send_response(estado)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.end_headers()
        self.wfile.write(json.dumps(datos).encode("utf-8"))

    # Ejecutar un comando y comprobar si hubo errores
    def comando(self, instrucciones, entrada=None, tiempo=30):
        proceso = subprocess.run(
            instrucciones,
            cwd=BASE,
            input=entrada,
            capture_output=True,
            text=True,
            timeout=tiempo
        )
        if proceso.returncode != 0:
            mensaje = (proceso.stderr + "\n" + proceso.stdout).strip()
            raise ValueError(mensaje or "Error al ejecutar un comando")
        return proceso.stdout

    # Recibir el programa BMLang desde la pagina
    def do_POST(self):
        try:
            if self.path != "/api/procesar":
                raise ValueError("Ruta desconocida")

            cantidad = int(self.headers.get("Content-Length", 0))
            if cantidad <= 0 or cantidad > 65536:
                raise ValueError("Solicitud vacia o demasiado grande")

            datos = json.loads(self.rfile.read(cantidad))
            accion = datos.get("accion")
            codigo = datos.get("codigo", "")
            entrada = datos.get("entrada", "")

            if accion not in ("intermedio", "nasm", "compilar", "ejecutar"):
                raise ValueError("Accion desconocida")
            if not isinstance(codigo, str) or not codigo.strip():
                raise ValueError("Escribe un programa primero")
            if not isinstance(entrada, str):
                raise ValueError("Entrada para READ no valida")

            # Evitar resultados viejos si la compilacion falla
            for archivo in ("codigo_intermedio.txt", "programa.asm", "programa.o", "programa"):
                ruta = BASE / archivo
                if ruta.exists():
                    ruta.unlink()

            # 1. Generar analizador con Bison y Flex
            self.comando(["bison", "-d", "analizador.y"])
            self.comando(["flex", "analizador.l"])
            self.comando([
                "gcc", "analizador.tab.c", "lex.yy.c", "-o", "analizador", "-lfl"
            ])

            # 2. Generar codigo de tres direcciones
            intermedio = self.comando([str(BASE / "analizador")], entrada=codigo, tiempo=5)
            if "Analisis sintactico correcto" not in intermedio:
                raise ValueError("El programa BMLang tiene errores de sintaxis")

            intermedio = intermedio.replace("Analisis sintactico correcto", "").strip()
            (BASE / "codigo_intermedio.txt").write_text(intermedio + "\n", encoding="utf-8")

            if accion == "intermedio":
                self.responder(200, {
                    "ok": True,
                    "mensaje": "Codigo intermedio generado",
                    "codigo": intermedio,
                    "detalle": "Analisis sintactico correcto"
                })
                return

            # 3. Traducir el codigo intermedio a NASM
            self.comando(["gcc", "-Wall", "Traductor_nasm.c", "-o", "traductor_nasm"])
            self.comando([str(BASE / "traductor_nasm")], tiempo=5)
            nasm = (BASE / "programa.asm").read_text(encoding="utf-8")

            if accion == "nasm":
                self.responder(200, {
                    "ok": True,
                    "mensaje": "Codigo NASM generado",
                    "codigo": nasm,
                    "detalle": "Traduccion a NASM correcta"
                })
                return

            # 4. Ensamblar y crear el ejecutable
            self.comando(["nasm", "-f", "elf32", "programa.asm", "-o", "programa.o"])
            self.comando(["ld", "-m", "elf_i386", "programa.o", "-o", "programa"])

            if accion == "compilar":
                self.responder(200, {
                    "ok": True,
                    "mensaje": "Ejecutable generado correctamente",
                    "codigo": nasm,
                    "detalle": "Archivo programa creado"
                })
                return

            # 5. Ejecutar el programa con los datos del campo READ
            salida = self.comando([str(BASE / "programa")], entrada=entrada, tiempo=5)
            self.responder(200, {
                "ok": True,
                "mensaje": "Programa ejecutado correctamente",
                "codigo": nasm,
                "detalle": salida if salida else "El programa termino sin mostrar resultados"
            })

        except subprocess.TimeoutExpired:
            self.responder(400, {
                "ok": False,
                "mensaje": "Tiempo de ejecucion agotado",
                "detalle": "Revisa si existe un ciclo infinito o faltan datos para READ"
            })
        except Exception as error:
            self.responder(400, {
                "ok": False,
                "mensaje": "No se pudo completar la solicitud",
                "detalle": str(error)
            })


# Iniciar el servidor
if __name__ == "__main__":
    servidor = HTTPServer(("127.0.0.1", 8000), Servidor)
    print("Abre http://127.0.0.1:8000")
    try:
        servidor.serve_forever()
    except KeyboardInterrupt:
        print("\nServidor detenido")
    finally:
        servidor.server_close()
