param(
    [string]$Ejecutable = 'build-review/Release/testores.exe',
    [string]$Salida = 'docs/resultados/ej5_bt_ascendente',
    [ValidateRange(1, 86400)][int]$LimiteSegundos = 60,
    [switch]$Forzar
)

# Reutiliza el mismo procedimiento de BT del punto 5.2.3, sobre las copias
# ordenadas del punto 5.2.2. No vuelve a medir ni reemplaza los otros órdenes.
& (Join-Path $PSScriptRoot 'medir_bt_original.ps1') -Ejecutable $Ejecutable -Salida $Salida `
    -Orden ascendente -LimiteSegundos $LimiteSegundos -Forzar:$Forzar
