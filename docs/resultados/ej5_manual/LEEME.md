# Serie manual del apartado 5.3

Serie histórica anterior a la integración de main (revisión 66804cb): BT
seleccionaba la primera fila no cubierta en S1. La versión corregida selecciona
la fila no cubierta cuyo último uno está más a la izquierda. Los tiempos BT
manuales no se atribuyen a la versión corregida; su serie automática queda
separada en `docs/resultados/experimentos.csv`.

`mediciones.csv` transcribe los tiempos comunicados por el usuario en el chat,
convertidos a milisegundos. Las series automáticas previas con límite de 60 s
se eliminaron del repositorio porque el reporte no las utiliza; la serie
reproducible con el código actual está en `docs/resultados/experimentos*`.

El usuario confirmó que las 19 h 14 min 43 s corresponden a BT sobre gamma3
en orden original y al tiempo reportado por el programa C++. La conversión es
69 283 s = 69 283 000 ms. No se verificó la equivalencia de las condiciones de
ese registro con las de la serie local: se marca con asterisco en el reporte y
no se utiliza para razones de rendimiento entre algoritmos.

Los siete valores sin resultado completo se conservan como `falta ejecutar`.
Las proyecciones de `analisis_5_3.tex` son escenarios geométricos no validados;
no se incorporan a la tabla de tiempos observados. Las de gamma dependen de
supuestos adicionales de comparabilidad y transferencia del crecimiento entre
órdenes, explicitados en el texto. No constituyen tiempos esperados fiables.

El entorno local se describe sólo con las especificaciones pertinentes
proporcionadas por el usuario: Intel Core Ultra 7 155H, RAM de 16 GB y Windows
de 64 bits. No se incorporan identificadores del dispositivo ni del producto.
