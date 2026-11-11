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

Primer paso del Ejercicio 5.1 (requiere guardar previamente `data/matriz_B.txt`):

```bash
./build/testores theta --entrada-a data/matriz_A.txt --entrada-b data/matriz_B.txt --salida data/theta_AB.txt
./build/testores bt --entrada data/theta_AB.txt
```

`theta` concatena cada fila de A con cada fila de B, manteniendo ese orden.
Las entradas deben ser básicas; no se reducen ni reordenan automáticamente.
Con A de 6 × 7 y B de 8 × 7 se obtiene una matriz básica de 48 × 14,
con 14 testores típicos.

El operador `phi` repite horizontalmente N copias de la entrada (N=1 es la
matriz original), conservando sus filas y su orden:

```bash
./build/testores phi --entrada data/theta_AB.txt --copias 2 --salida data/phi2_theta_AB.txt
```

Para las matrices de este proyecto, φ^N(θ(A,B)) tiene 48 filas, 14N columnas y
5N² + 7N³ + 2N⁴ testores típicos. La tabla 5 utiliza N=1,2,3,4,5.
El operador `gamma` construye N copias diagonales y rellena con ceros:

```bash
./build/testores gamma --entrada data/theta_AB.txt --copias 2 --salida data/gamma2_theta_AB.txt
```

γ^N(θ(A,B)) tiene 48N filas, 14N columnas y 14^N testores típicos.
La tabla 6 utiliza N=1,2,3,4. Con N=1, tanto φ como γ devuelven θ(A,B).
Para el punto 5.2.1, YYC en orden original (PowerShell, desde la raíz):

```powershell
.\scripts\medir_yyc_original.ps1
```

El script mide una ejecución sin traza por matriz, verifica las cantidades de
testores contra las fórmulas y conserva el orden de los archivos. Genera las
tablas `tabla_phi.csv/.tex` y `tabla_gamma.csv/.tex`, evidencias por fila y la
metodología en `docs/resultados/ej5_yyc_original/`. N=1 comparte la medición de
θ(A,B) en ambas tablas. El tiempo es la suma de los intervalos de procesamiento
de las filas, excluyendo impresión y exportación. Para repetir y reemplazar
esas mediciones se utiliza `-Forzar`; `-Salida` permite otra carpeta.
Para el punto 5.2.2, YYC con filas ordenadas por cantidad ascendente de unos:

```powershell
.\scripts\medir_yyc_ascendente.ps1
```

El script guarda copias ordenadas en `docs/resultados/ej5_yyc_ascendente/matrices/`.
En empates mantiene el orden original. Mide una ejecución original y otra
ascendente por matriz, con el mismo ejecutable y en la misma sesión, sin
reemplazar las evidencias del punto 5.2.1. El ordenamiento queda fuera del tiempo
medido. Verifica que las filas no cambien y que ambos órdenes produzcan exactamente
los mismos testores típicos (no sólo la misma cantidad). Exporta tablas comparativas
CSV/LaTeX y evidencias completas a `docs/resultados/ej5_yyc_ascendente/`.
Para repetir estas mediciones se utiliza `-Forzar`; `-Salida` permite otra carpeta.
Para el punto 5.2.3, BT en orden original:

```powershell
.\scripts\medir_bt_original.ps1
```

El script intenta una ejecución sin traza por matriz y conserva las opciones
predeterminadas del algoritmo. Cada proceso tiene un límite de 60 segundos,
configurable con `-LimiteSegundos`. Los casos completos incluyen el tiempo de
`BtSolver::solve` y la comparación exacta con los testores de YYC. Los casos
interrumpidos se registran como `limite_proceso`, sin tiempo final del algoritmo
ni resultado completo. El límite cubre todo el proceso (incluida la verificación
posterior), así que no se presenta como una cota del tiempo del algoritmo.
Las tablas CSV/LaTeX, salidas y metodología quedan en
`docs/resultados/ej5_bt_original/`. `-Forzar` repite y reemplaza estas mediciones;
`-Salida` permite conservar otra serie de resultados.
Para el punto 5.2.4, BT con filas en orden ascendente:

```powershell
.\scripts\medir_bt_ascendente.ps1
```

Utiliza las copias ordenadas del punto 5.2.2, tras comprobar que conservan todas
las filas y están en orden ascendente por cantidad de unos. Emplea el mismo
procedimiento, opciones y límite de 60 segundos por proceso que el punto 5.2.3.
El ordenamiento no forma parte del tiempo medido. Los resultados quedan en
`docs/resultados/ej5_bt_ascendente/`, con igualdad exacta contra YYC en los casos
completos y estado explícito para los casos interrumpidos. Admite los mismos
parámetros `-LimiteSegundos`, `-Forzar`, `-Salida` y `-Ejecutable`.

Para reunir las cinco columnas de matrices y las cuatro columnas de tiempos
del punto 5.2 en tablas CSV/LaTeX:

```powershell
.\scripts\unir_tablas_5_2.ps1
```

Las tablas quedan en `docs/resultados/ej5_comparacion/`. Antes de reunirlas se
verifica la identidad de las matrices mediante sus hashes y la correspondencia
de dimensiones y conteos. Los tiempos de BT sin finalización se muestran como
`ND`, conservando los estados y límites en sus evidencias originales.

Para ordenar una matriz individual o guardar todos los testores finales de YYC:

```powershell
.\build-review\Release\testores.exe ordenar --entrada data/theta_AB.txt --salida data/theta_AB_ascendente.txt
.\build-review\Release\testores.exe yyc --entrada data/theta_AB.txt --testores-salida resultados/testores_theta.txt
```

BT también acepta `--testores-salida` para exportar sólo los testores típicos
finales como filas binarias, sin necesitar exportar toda la lista de testores.
En Windows con Visual Studio, el ejecutable está en `build/Release/testores.exe`
(o `build-review/Release/testores.exe` para la compilación de revisión).

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
