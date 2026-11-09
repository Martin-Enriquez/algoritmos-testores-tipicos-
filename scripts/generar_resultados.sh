#!/usr/bin/env sh
# Regenera todos los datos y resultados usados en el reporte (Ejercicios 1 al 5).
# Uso (desde la raíz del repositorio, después de compilar):  sh scripts/generar_resultados.sh
set -eu

BUILD_DIRECTORY="${BUILD_DIRECTORY:-build}"
BIN="${BIN:-$BUILD_DIRECTORY/testores}"
[ -x "$BIN" ] || BIN="$BIN.exe"
EXPERIMENTOS="${EXPERIMENTOS:-$BUILD_DIRECTORY/testores_experimentos}"
[ -x "$EXPERIMENTOS" ] || EXPERIMENTOS="$EXPERIMENTOS.exe"
PYTHON="${PYTHON:-python3}"
[ -x "$BIN" ] && [ -x "$EXPERIMENTOS" ] || { echo "Compile ambos ejecutables antes de regenerar resultados." >&2; exit 1; }
command -v "$PYTHON" >/dev/null 2>&1 || { echo "No se encuentra Python; indique PYTHON=/ruta/al/interprete." >&2; exit 1; }
OUT=docs/resultados
mkdir -p data "$OUT"

# Ejercicio 1: matriz aleatoria 100 x 10, su matriz básica y densidad.
"$BIN" generar --filas 100 --columnas 10 --semilla 2026 \
    --salida data/ej1_aleatoria_100x10.txt --salida-basica data/ej1_basica_100x10.txt --forzar \
    > "$OUT/ej1_100x10.txt"

# Ejercicio 2.1: matriz básica A (>= 7 columnas, densidad >= 0.4).
"$BIN" generar --filas 8 --columnas 7 --semilla 2026 \
    --salida data/matriz_A_original.txt --forzar > /dev/null
"$BIN" buscar --columnas 7 --filas 8 --densidad-min 0.4 --min-filas-basica 6 --max-filas-basica 6 \
    --semilla 2026 --salida data/matriz_A.txt --forzar > "$OUT/ej2_1_busqueda_A.txt"

# Ejercicio 2.2: traza detallada de YYC sobre A.
"$BIN" yyc --entrada data/matriz_A.txt --traza --completo > "$OUT/yyc_A_traza.txt"

# Ejercicio 2.4: tabla con mediana de 10000 ejecuciones.
"$BIN" yyc --entrada data/matriz_A.txt --repeticiones 10000 --completo \
    --csv "$OUT/yyc_A.csv" --latex "$OUT/yyc_A_tabla.tex" --latex-matriz "$OUT/matriz_A.tex" --forzar \
    > "$OUT/yyc_A_tabla.txt"

# Ejercicio 3.1: traza detallada de BT sobre A (texto y tabla LaTeX).
"$BIN" bt --entrada data/matriz_A.txt --traza --completo --latex-traza "$OUT/bt_A_traza.tex" --forzar \
    > "$OUT/bt_A_traza.txt"

# Ejercicio 3.3: testores encontrados, típicos y tiempo total (mediana de 10000 ejecuciones).
"$BIN" bt --entrada data/matriz_A.txt --repeticiones 10000 --completo \
    --csv "$OUT/bt_A.csv" --latex "$OUT/bt_A_resumen.tex" --forzar > "$OUT/bt_A.txt"

REPETICIONES="${REPETICIONES:-9}"
LIMITE_MS="${LIMITE_MS:-10000}"
"$EXPERIMENTOS" --repeticiones "$REPETICIONES" --limite-ms "$LIMITE_MS" --salida "$OUT" --forzar \
    > "$OUT/experimentos_log.txt"
cp "$OUT/matriz_B.txt" data/matriz_B.txt
"$BIN" bt --entrada data/matriz_B.txt --repeticiones 10000 --completo \
    --csv "$OUT/bt_B.csv" --latex "$OUT/bt_B_resumen.tex" --forzar \
    > "$OUT/bt_B.txt"
"$BIN" yyc --entrada data/matriz_B.txt --latex-matriz "$OUT/matriz_B.tex" --forzar > "$OUT/yyc_B.txt"
"$BIN" bt --entrada data/matriz_B.txt --traza --completo > "$OUT/bt_B_traza.txt"
"$PYTHON" scripts/actualizar_metadatos.py --build "$BUILD_DIRECTORY" --repeticiones "$REPETICIONES" --limite-ms "$LIMITE_MS"
echo "Resultados generados en $OUT/"
