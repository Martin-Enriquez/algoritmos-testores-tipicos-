# Algoritmos para el Cálculo de Testores Típicos en Matrices Básicas

Proyecto de **Matemáticas Discretas (USFQ, 202610)**: implementación, evaluación y comparación de algoritmos para encontrar todos los testores típicos de una matriz básica.

| Ejercicio | Estado | Dónde |
|---|---|---|
| 1. Generador de matrices, matriz básica y densidad | ✅ | `testores generar`, `testores basica`, menú 1 |
| 2.1 Matriz básica A (≥ 7 columnas, densidad ≥ 0.4) | ✅ | `data/matriz_A.txt` (6 × 7, densidad 0.4762) |
| 2.2 Ejecución manual de YYC | ✅ | `docs/reporte.tex` §4.3 y `docs/resultados/yyc_A_traza.txt` |
| 2.3 Implementación de YYC (Ψ* y tiempo acumulado por fila) | ✅ | `testores yyc`, menú 6–7 |
| 2.4 Tabla de resultados sobre A | ✅ | `docs/resultados/yyc_A_tabla.tex`, `yyc_A.csv` |
| 3.1 Ejecución manual de BT sobre A | ✅ | `docs/reporte.tex` §5.2 y `docs/resultados/bt_A_traza.txt` |
| 3.2 Implementación de BT (testores, típicos y tiempo total) | ✅ | `testores bt`, menú 10–12 |
| 3.3 Resultados de BT sobre A | ✅ | `docs/resultados/bt_A_resumen.tex`, `bt_A.csv` |
| 4. Matriz B | ⏳ | — |
| 5. Operadores φ, θ, γ y experimentos | ⏳ | — |

## Compilación

Requisitos: compilador C++17 (GCC ≥ 9, Clang ≥ 10 o MSVC 2019+) y CMake ≥ 3.16.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure    # 63 pruebas
```

En Windows con MSYS2 (UCRT64) agregue `C:\msys64\ucrt64\bin` al `PATH` y use `-G Ninja`.

## Uso

```bash
./build/testores                 # menú interactivo
./build/testores ayuda           # lista de subcomandos

# Ejercicio 1: matriz 100 x 10, su básica y densidad
./build/testores generar --filas 100 --columnas 10 --semilla 2026 --salida-basica data/basica.txt

# Ejercicio 2.1: buscar una matriz básica con >= 7 columnas y densidad >= 0.4
./build/testores buscar --columnas 7 --filas 8 --densidad-min 0.4 --semilla 2026 --salida data/A.txt

# Ejercicios 2.2-2.4: YYC sobre A
./build/testores yyc --entrada data/matriz_A.txt --traza            # paso a paso explicado
./build/testores yyc --entrada data/matriz_A.txt --repeticiones 10000 \
    --csv out.csv --latex out.tex                                   # tabla con mediana de tiempos

# Ejercicios 3.1-3.3: BT sobre A
./build/testores bt --entrada data/matriz_A.txt --traza             # cada n-uplo α y cada salto
./build/testores bt --entrada data/matriz_A.txt --repeticiones 10000 \
    --csv bt.csv --latex bt.tex                                     # testores típicos y tiempo total
```

Todos los resultados del reporte se regeneran con `sh scripts/generar_resultados.sh`.

**Formato de archivo de matriz:** una fila por línea con `0`/`1`; separadores opcionales (espacio, `,`, `;`); `#` inicia un comentario.

## Arquitectura

Arquitectura limpia en cuatro capas; cada una es una librería CMake y sólo puede depender de las capas internas:

```
presentation  ──►  infrastructure  ──►  application  ──►  domain
(CLI, menú)        (archivos, azar,      (casos de uso,     (matrices, matriz básica,
                    reloj, CSV/LaTeX)     puertos)           testores, YYC, BT)
```

```
include/testores/<capa>/   cabeceras públicas
src/<capa>/                implementación
apps/main.cpp              punto de entrada
tests/                     pruebas unitarias (sin dependencias externas)
data/                      matrices usadas en el reporte
docs/                      reporte LaTeX y resultados generados
scripts/                   regeneración de resultados
```

Decisiones principales:

- `domain::YycSolver` es **incremental** (`processNextRow()`), así la capa de aplicación mide sólo el tiempo del algoritmo y la impresión de Ψ* queda fuera de la medición.
- `ColumnSet` es un bitset de tamaño fijo (≤ 512 columnas) sin memoria dinámica: YYC sobre A pasó de ~88 µs a ~7 µs.
- La generación usa `mt19937_64` con una conversión portable a Bernoulli: la misma semilla produce la misma matriz con cualquier compilador.
- BT (`domain/bt.hpp`) recorre los n-uplos α como números binarios (x1 = bit más significativo) con los saltos S1 (α no es testor: 1 en el último 1 de la primera fila de ceros y ceros a su derecha) y S2 (α es testor: sumar 1 en la posición de su último 1). La tipicidad se decide durante la ejecución (fila típica de cada rasgo) y se verifica a posteriori (testores minimales entre los encontrados). BT **no** reordena filas: el orden de las filas cambia los saltos, que es lo que estudia el Ejercicio 5.

## Seguridad

Ver [SECURITY.md](SECURITY.md).

## Integrantes

Martín Enríquez, Paula Gallardo, Eduardo Tapia. Profesor: Eduardo Alba.
