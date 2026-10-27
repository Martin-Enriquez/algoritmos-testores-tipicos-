# Algoritmos para el Cálculo de Testores Típicos en Matrices Básicas

Proyecto de **Matemáticas Discretas (USFQ, 202610)**: implementación, evaluación y comparación de algoritmos para encontrar todos los testores típicos de una matriz básica.

| Ejercicio | Estado | Dónde |
|---|---|---|
| 1. Generador de matrices, matriz básica y densidad | ✅ | `testores generar`, `testores basica`, menú 1 |
| 2.1 Matriz básica A (≥ 7 columnas, densidad ≥ 0.4) | ✅ | `data/matriz_A.txt` (6 × 7, densidad 0.4762) |
| 2.2 Ejecución manual de YYC | ✅ | `docs/reporte.tex` §4.3 y `docs/resultados/yyc_A_traza.txt` |
| 2.3 Implementación de YYC (Ψ* y tiempo acumulado por fila) | ✅ | `testores yyc`, menú 6–7 |
| 2.4 Tabla de resultados sobre A | ✅ | `docs/resultados/yyc_A_tabla.tex`, `yyc_A.csv` |
| 3. BT | ⏳ | — |
| 4. Matriz B | ⏳ | — |
| 5. Operadores φ, θ, γ y experimentos | ⏳ | — |

## Compilación

Requisitos: compilador C++17 (GCC ≥ 9, Clang ≥ 10 o MSVC 2019+) y CMake ≥ 3.16.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure    # 49 pruebas
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
```

Todos los resultados del reporte se regeneran con `sh scripts/generar_resultados.sh`.

**Formato de archivo de matriz:** una fila por línea con `0`/`1`; separadores opcionales (espacio, `,`, `;`); `#` inicia un comentario.

## Arquitectura

Arquitectura limpia en cuatro capas; cada una es una librería CMake y sólo puede depender de las capas internas:

```
presentation  ──►  infrastructure  ──►  application  ──►  domain
(CLI, menú)        (archivos, azar,      (casos de uso,     (matrices, matriz básica,
                    reloj, CSV/LaTeX)     puertos)           testores, YYC)
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
- Para añadir BT (Ejercicio 3) basta con un nuevo `domain/bt.hpp` y su caso de uso, reutilizando `BooleanMatrix`, `ColumnSet` y `domain/testor.hpp`.

## Seguridad

Ver [SECURITY.md](SECURITY.md).

## Integrantes

Martín Enríquez, Paula Gallardo, Eduardo Tapia. Profesor: Eduardo Alba.
