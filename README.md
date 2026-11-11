# Algoritmos para calcular testores típicos

Proyecto de Matemáticas Discretas, USFQ, semestre 202610. Implementa los ejercicios 1 a 5: matrices booleanas y básicas, YYC, BT, dualidad y experimentos con los operadores del artículo de Alba-Cabrera et al. (2016). El ejercicio 6 corresponde a grupos de cuatro integrantes; este grupo tiene tres.

| Ejercicio | Implementación y evidencia |
|---|---|
| 1 | Generación reproducible, reducción básica que conserva la fila nula y densidad |
| 2 | A de 6 × 7, densidad 20/42; ejecución manual de YYC, estados por fila y tiempos acumulados |
| 3 | BT con la selección de fila del apunte, traza de 44 candidatos y 8 típicos |
| 4 | B de 8 × 7; prueba de basicidad y dualidad; BT recupera las 6 filas de A |
| 5 | Operadores φ, θ y γ; nueve matrices; comparación original/ascendente, tablas y análisis |

Las tablas distinguen los tiempos completados de las ejecuciones que alcanzan el límite. Una celda `limite` en el CSV, representada por una cota en el informe, no es una mediana de terminación. Las primeras cinco columnas se conocen exactamente por las propiedades de los operadores.

## Compilar y verificar

Requisitos: CMake ≥ 3.16 y compilador C++17 (GCC, Clang o MSVC). El programa y las pruebas no requieren bibliotecas externas. Python ≥ 3.10 se usa para los metadatos del informe y el oráculo independiente; LaTeX permite recompilar el PDF.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

En Windows con MSYS2 UCRT64 y Ninja:

```powershell
$env:Path = 'C:\msys64\ucrt64\bin;' + $env:Path
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

Con Visual Studio los ejecutables quedan en `build/Release/`; con Ninja/Make, en `build/`. CTest ejecuta la suite C++ y una comprobación de dependencias entre capas. La suite incluye las 9.418 matrices con hasta cuatro filas y cuatro columnas y como máximo doce entradas, matrices aleatorias, fila nula, cancelación y límites de recursos. La integración continua verifica Linux, Windows y sanitizadores en Linux. Para sanitizadores locales use `-DTESTORES_SANITIZERS=ON` en una compilación Debug de Linux/macOS.

## Uso

```powershell
.\build\testores.exe
.\build\testores.exe ayuda
.\build\testores.exe generar --filas 100 --columnas 10 --semilla 2026 --salida-basica data/basica.txt
.\build\testores.exe basica --entrada data/matriz_A_original.txt
.\build\testores.exe yyc --entrada data/matriz_A.txt --traza --completo
.\build\testores.exe bt --entrada data/matriz_A.txt --traza --completo
.\build\testores.exe bt --entrada data/matriz_B.txt --repeticiones 10000
```

En Linux/macOS use `./build/testores`. Los archivos contienen una fila por línea con `0`/`1`; admiten espacios, comas, punto y coma y comentarios desde `#`. Sobrescribir requiere `--forzar` o confirmación en el menú.

BT visita 24 testores de A, pero A tiene **86 testores totales**: los saltos omiten otros. Los 8 típicos son completos. En B visita 20 de 42 testores y encuentra los 6 típicos.

## Experimentos y reproducción

El segundo ejecutable construye B y C = θ(A,B), genera φᴺ(C), N = 1…5, y γᴺ(C), N = 1…4. Contrasta los conjuntos en todas las ejecuciones completas y con las cardinalidades teóricas. Guarda matrices, CSV y tablas LaTeX.

```powershell
.\build\testores_experimentos.exe --repeticiones 9 --limite-ms 10000 --salida docs/resultados --forzar
.\build\testores_experimentos.exe --familia phi --n 1 --repeticiones 9 --salida demo
```

Se cronometra `solve` completo, con inicialización y ordenación final; se excluyen generación de matrices, ordenamiento previo, validación y escritura. Hay un calentamiento y el límite se aplica a cada ejecución. Las consultas de cancelación están incluidas. El CSV conserva estado, repeticiones completas, candidatos BT y motivo de interrupción.

La mediana par promedia los dos valores centrales, truncando al nanosegundo. La tabla YYC de A calcula la mediana del **acumulado** hasta cada fila y sólo cronometra su procesamiento incremental. Para comparar algoritmos con igual alcance se usan las tablas del ejercicio 5.

Ordenar puede cambiar los estados intermedios de YYC. En BT, la regla del último 1 más a la izquierda mantiene la secuencia de candidatos bajo permutaciones de filas, aunque el tiempo de las verificaciones puede cambiar. Los tiempos dependen del equipo y su carga.

Para regenerar todo en Windows, con Python y LaTeX disponibles:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/generar_resultados.ps1
python scripts/verificar_resultados.py
powershell -ExecutionPolicy Bypass -File scripts/compilar_reporte.ps1
```

Se admiten `-Python C:\ruta\python.exe` y `-PdfLatex C:\ruta\pdflatex.exe`. El primer script también acepta `-BuildDirectory`, `-Repeticiones` y `-LimiteMs`. La regeneración completa puede tardar varios minutos.

En Linux/macOS:

```sh
sh scripts/generar_resultados.sh
python3 scripts/verificar_resultados.py
cd docs
pdflatex -interaction=nonstopmode -halt-on-error reporte.tex
pdflatex -interaction=nonstopmode -halt-on-error reporte.tex
```

El shell admite `BIN`, `EXPERIMENTOS`, `PYTHON`, `REPETICIONES` y `LIMITE_MS`. Los metadatos registran el entorno real. La fecha de culminación es explícita en el LaTeX.

## Organización y arquitectura

| Carpeta | Responsabilidad |
|---|---|
| `apps/` | Composición y puntos de entrada de ambos ejecutables |
| `include/testores/<capa>/` | Cabeceras y contratos públicos de cada capa |
| `src/domain/` | Matrices, conjuntos, reducción, testores, YYC, BT y operadores |
| `src/application/` | Casos de uso, mediciones, experimentos y puertos |
| `src/infrastructure/` | Archivos, CSV/LaTeX, reloj y generador aleatorio |
| `src/presentation/` | Argumentos, menú y vistas de consola |
| `tests/` | Pruebas C++ sin bibliotecas externas |
| `cmake/` | Comprobación de arquitectura |
| `data/` | Entradas reproducibles A, B y ejercicio 1 |
| `docs/` | Informe académico LaTeX y PDF |
| `docs/resultados/` | Trazas, matrices experimentales, tiempos, tablas y verificaciones |
| `scripts/` | Regeneración, metadatos, oráculo independiente y compilación PDF |
| `.github/workflows/` | Integración continua |

```text
presentation ──► application ──► domain
infrastructure ────────▲
apps compone presentation e infrastructure
```

El dominio no conoce archivos, consola ni reloj del sistema. Aplicación define los puertos `Clock`, `RandomSource`, `MatrixRepository`, `ResultExporter` y `RandomSourceFactory`. Infraestructura los implementa; `apps/main.cpp` los inyecta en `CliServices`. `cmake/CheckArchitecture.cmake` comprueba las dependencias permitidas. Los límites se detallan en [SECURITY.md](SECURITY.md).

## Integrantes

Martín Enríquez, Paula Gallardo y Eduardo Tapia. Profesor: Eduardo Alba.

## Desarrollo y serie manual conservados de ejercicio_4

El reporte principal conserva el desarrollo detallado de los ejercicios 4 y 5
preparado en esta rama e incorpora las correcciones matemáticas de main.
Los tiempos de `docs/resultados/ej5_manual/` pertenecen a la versión previa
que elegía la primera fila sin cubrir en BT; son una serie histórica y no
se presentan como mediciones de la implementación corregida. Las tablas del
nuevo ejecutable están en `docs/resultados/experimentos*.tex` y `experimentos.csv`.

Se conservan los comandos individuales `theta`, `phi`, `gamma`, `ordenar` y
la exportación `--testores-salida` de YYC y BT. La presentación accede a los
archivos mediante los puertos inyectados en `CliServices`.

```powershell
.\scripts\generar_ejercicio_5_1.ps1
```

Este script genera las familias en `data/ej5_1/` y sus comprobaciones en
`docs/resultados/ej5_1/`. Para repetirlo se puede indicar otra salida o `-Forzar`.
Los nombres `phiPower`, `gammaPower` y `sortRowsByOnes` siguen disponibles;
`repeatPhi`, `repeatGamma` y `sortRowsAscending` permiten usar el nuevo
módulo de experimentos sin romper los comandos y pruebas de esta rama.
