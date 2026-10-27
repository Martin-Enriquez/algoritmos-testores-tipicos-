# Consideraciones de seguridad

`testores` es una herramienta local de línea de comandos, pero procesa archivos y entradas que podrían venir de terceros. Estas son las medidas implementadas:

## Validación de entradas

- **Números** (`presentation/console_input.cpp`): se convierten con `std::from_chars` exigiendo que toda la cadena sea válida y esté en rango. Se rechazan `NaN`, infinitos y desbordamientos. En el menú una entrada inválida vuelve a preguntarse; nunca queda `std::cin` en estado de error.
- **Líneas interactivas**: longitud máxima de 4096 caracteres. El fin de entrada (EOF) termina el programa de forma ordenada.
- **Argumentos**: cada subcomando tiene una lista blanca de opciones; una opción desconocida, repetida o sin valor es un error, nunca se ignora.

## Archivos

- **Lectura** (`infrastructure/matrix_file.cpp`): sólo archivos regulares de hasta 16 MiB; sólo se aceptan `0`, `1`, separadores y comentarios; todas las filas deben tener el mismo ancho. Los errores indican el número de línea.
- **Escritura**: nunca se sobrescribe un archivo existente sin `--forzar` o confirmación explícita en el menú. Los comentarios que se escriben en el encabezado se limpian de saltos de línea para que no puedan inyectar filas.

## Límites de recursos

| Recurso | Límite | Motivo |
|---|---|---|
| Filas | 100 000 | memoria y tiempo de la reducción O(m²) |
| Columnas | 512 | tamaño fijo de `ColumnSet` |
| \|Ψ*\| en YYC | 2 000 000 (~160 MB) | el número de testores puede crecer exponencialmente |
| Instantáneas de Ψ* por fila | 200 000 conjuntos en total | evita memoria O(m·\|Ψ*\|) |
| Repeticiones de medición | 100 000 | evita ejecuciones interminables |

Superar un límite produce un error claro (`YycResourceLimitExceeded`, `std::invalid_argument`…), nunca un fallo de memoria.

## Compilación endurecida

- GCC/Clang: `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wold-style-cast -Wnull-dereference`, `-fstack-protector-strong`, `_FORTIFY_SOURCE=2`, `_GLIBCXX_ASSERTIONS`. El código compila sin advertencias.
- MSVC: `/W4 /permissive- /sdl`.
- Opción `-DTESTORES_SANITIZERS=ON` para AddressSanitizer + UBSan (Linux/macOS).

## Otros

- Sin `rand()`/`srand()`: se usa `std::mt19937_64` con semilla de `std::random_device`. No se usa con fines criptográficos.
- Acceso a bits con verificación de rango (`ColumnSet::test/set` lanzan `std::out_of_range`).
- Sin dependencias externas en tiempo de ejecución ni de pruebas.
