# Medir BT en Google Colab

Archivos para subir:

- `medir_bt.ipynb`: cuaderno que se abre con **Archivo > Subir cuaderno** en Colab.
- `testores_bt_colab.zip`: código C++ y las matrices necesarias. Se sube al ejecutar la primera celda de cada cuaderno.
- `medir_bt_phi5.ipynb`, `medir_bt_gamma3.ipynb` y `medir_bt_gamma4.ipynb`: cuadernos independientes para las últimas tres matrices. Cada uno usa el mismo ZIP y mide sus dos órdenes en secuencia.
- `medir_bt_phi2.ipynb`: cuaderno de prueba para φ², con el mismo flujo y los dos órdenes.
- `medir_bt_phi4_original.ipynb`: cuaderno para repetir únicamente φ⁴ original, sin límite de tiempo interno.

El ZIP no es un ejecutable de Windows. La segunda celda compila el código y crea `testores` para Linux en `/content/testores_bt_colab/build-colab/testores`.

En la celda titulada **Ejecutar una medición**, cambia `CASO` y `ORDEN` para cada pareja:

| CASO | ORDEN |
|---|---|
| `phi4` | `original` |
| `phi4` | `ascendente` |
| `phi5` | `original` |
| `phi5` | `ascendente` |
| `gamma3` | `original` |
| `gamma3` | `ascendente` |
| `gamma4` | `original` |
| `gamma4` | `ascendente` |

Puedes ejecutar una pareja a la vez y descargar su registro `.txt`, o usar la celda final **Ejecutar las cuatro matrices automáticamente**. Esta última hace las ocho combinaciones en orden consecutivo y descarga `resultados_bt_colab.zip` con los registros y `resumen_bt_colab.csv` cuando concluye. No ejecutes las dos opciones a la vez.

Para los cuadernos independientes, abre cada `.ipynb` por **Archivo > Subir cuaderno**. En cada uno, ejecuta la primera celda y sube una copia de `testores_bt_colab.zip`; ejecuta la segunda para compilar y la última para medir. Cada cuaderno descarga un registro tras cada corrida y un resumen CSV al final. Si ejecutas varios cuadernos simultáneamente, la disponibilidad de entornos separados depende de Colab y de tu cuenta; los tiempos se obtendrán bajo esas condiciones de carga.

Los cuadernos no imponen un límite de tiempo por corrida. Si una sesión de Colab termina antes de que BT finalice, esa medición queda pendiente y pueden perderse los resultados aún no descargados. No compares directamente los tiempos de Colab con los de Windows: corresponden a máquinas distintas. Para estudiar el efecto del orden, compara ambos órdenes medidos en el mismo entorno.
