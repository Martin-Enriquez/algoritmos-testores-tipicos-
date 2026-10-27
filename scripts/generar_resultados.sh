#!/usr/bin/env sh
# Regenera todos los datos y resultados usados en el reporte (Ejercicios 1 y 2).
# Uso (desde la raíz del repositorio, después de compilar):  sh scripts/generar_resultados.sh
set -eu

BIN="${BIN:-build/testores}"
[ -x "$BIN" ] || BIN="build/testores.exe"
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

echo "Resultados generados en $OUT/"
