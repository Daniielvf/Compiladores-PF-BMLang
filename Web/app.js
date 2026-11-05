"use strict";

// buscar elementos de la pagina
const buscar = id => document.getElementById(id);
const editor = buscar("editor");
const archivo = buscar("archivo");
const nombre = buscar("nombre");
const resultado = buscar("resultado");
const salida = buscar("salida");
const estado = buscar("estado");

editor.value = "MAIN {\n    DEC $a;\n    $a => 2 + 3;\n    WRITE $a;\n}";

// limpiar los resultados
function limpiar() {
  resultado.textContent = "";
  salida.textContent = "";
}

// crear un archivo nuevo
buscar("nuevo").onclick = () => {
  if (!confirm("¿Limpiar el editor? Guarda tus cambios primero.")) return;
  editor.value = "";
  nombre.value = "programa.bml";
  limpiar();
};

// abrir un archivo
buscar("abrir").onclick = () => archivo.click();
archivo.onchange = async () => {
  const seleccionado = archivo.files[0];
  archivo.value = "";
  if (!seleccionado) return;
  if (!seleccionado.name.toLowerCase().endsWith(".bml")) return alert("Selecciona un archivo .bml");
  if (!confirm("¿Reemplazar el editor? Guarda tus cambios primero.")) return;
  editor.value = await seleccionado.text();
  nombre.value = seleccionado.name;
  limpiar();
};

// descargar el archivo
buscar("guardar").onclick = () => {
  const enlace = document.createElement("a");
  const destino = nombre.value.trim() || "programa.bml";
  enlace.download = destino.toLowerCase().endsWith(".bml") ? destino : destino + ".bml";
  enlace.href = URL.createObjectURL(new Blob([editor.value], { type: "text/plain;charset=utf-8" }));
  document.body.append(enlace);
  enlace.click();
  enlace.remove();
  setTimeout(() => URL.revokeObjectURL(enlace.href), 1000);
};

// enviar el codigo al servidor
async function procesar(accion) {
  const botones = document.querySelectorAll("button");
  botones.forEach(boton => boton.disabled = true);
  editor.readOnly = true;
  limpiar();
  estado.textContent = "Procesando";
  try {
    const respuesta = await fetch("/api/procesar", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ accion, codigo: editor.value, entrada: buscar("entrada").value })
    });
    const datos = await respuesta.json();
    resultado.textContent = datos.codigo || "";
    salida.textContent = datos.detalle || "";
    estado.textContent = datos.mensaje;
    estado.dataset.error = String(!datos.ok);
  } catch {
    estado.textContent = "No se pudo conectar con el servidor";
  } finally {
    botones.forEach(boton => boton.disabled = false);
    editor.readOnly = false;
  }
}

// conectar los botones
for (const accion of ["intermedio", "nasm", "compilar", "ejecutar"]) {
  buscar(accion).onclick = () => procesar(accion);
}
buscar("integrantes").onclick = () => buscar("equipo").showModal();
editor.oninput = limpiar;

function mostrarIntegrantes() {
    document.getElementById("ventanaIntegrantes").style.display = "block";
}

function cerrarIntegrantes() {
    document.getElementById("ventanaIntegrantes").style.display = "none";
}