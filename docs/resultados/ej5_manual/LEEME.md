# Serie manual del apartado 5.3

`mediciones.csv` transcribe los tiempos comunicados por el usuario en el chat,
convertidos a milisegundos. No reemplaza las series automáticas de las carpetas
`ej5_yyc_original`, `ej5_yyc_ascendente`, `ej5_bt_original` y `ej5_bt_ascendente`.

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
