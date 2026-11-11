param(
    [string]$Salida = 'docs/resultados/ej5_comparacion',
    [switch]$Forzar
)

$ErrorActionPreference = 'Stop'
Push-Location -LiteralPath (Split-Path -Parent $PSScriptRoot)
try {
    if ((Test-Path -LiteralPath $Salida) -and -not $Forzar) {
        throw "La carpeta $Salida ya existe. Use -Forzar para reemplazar las tablas."
    }
    # Comprobar ambas familias antes de escribir, sin volver a medir algoritmos.
    foreach ($family in @('phi', 'gamma')) {
        foreach ($source in @('ej5_yyc_ascendente', 'ej5_bt_original', 'ej5_bt_ascendente')) {
            if (-not (Test-Path -LiteralPath "docs/resultados/$source/tabla_${family}.csv")) {
                throw "Falta la tabla de $source, familia $family"
            }
        }
    }
    $yycMethod = Get-Content -LiteralPath 'docs/resultados/ej5_yyc_ascendente/metodologia.json' -Raw -Encoding utf8 | ConvertFrom-Json
    New-Item -ItemType Directory -Path $Salida -Force | Out-Null
    foreach ($family in @('phi', 'gamma')) {
        $yyc = @(Import-Csv -LiteralPath "docs/resultados/ej5_yyc_ascendente/tabla_${family}.csv" -Encoding utf8)
        $btOriginal = @(Import-Csv -LiteralPath "docs/resultados/ej5_bt_original/tabla_${family}.csv" -Encoding utf8)
        $btAscending = @(Import-Csv -LiteralPath "docs/resultados/ej5_bt_ascendente/tabla_${family}.csv" -Encoding utf8)
        $expectedRows = if ($family -eq 'phi') { 5 } else { 4 }
        if ($yyc.Count -ne $expectedRows -or $btOriginal.Count -ne $expectedRows -or $btAscending.Count -ne $expectedRows) {
            throw "Las tablas no tienen todos los casos de $family"
        }
        $table = @(foreach ($row in $yyc) {
            $original = @($btOriginal | Where-Object { $_.N -eq $row.N })
            $ascending = @($btAscending | Where-Object { $_.N -eq $row.N })
            if ($original.Count -ne 1 -or $ascending.Count -ne 1) { throw 'Caso duplicado o ausente en BT.' }
            foreach ($bt in @($original[0], $ascending[0])) {
                if ($bt.matriz -ne $row.matriz -or $bt.filas -ne $row.filas -or
                    $bt.columnas -ne $row.columnas -or $bt.testores_esperados -ne $row.testores_tipicos) {
                    throw "Las matrices no coinciden entre las tablas: $($row.matriz)"
                }
                if ($bt.estado -notin @('completado', 'limite_proceso')) { throw 'Estado de BT no reconocido.' }
            }
            if ($original[0].estado -eq 'completado' -and [string]::IsNullOrWhiteSpace($original[0].bt_original_ms)) { throw 'Falta el tiempo original de BT.' }
            if ($ascending[0].estado -eq 'completado' -and [string]::IsNullOrWhiteSpace($ascending[0].bt_ascendente_ms)) { throw 'Falta el tiempo ascendente de BT.' }
            $key = if ([int]$row.N -eq 1) { 'theta_AB' } else { "${family}$($row.N)_theta_AB" }
            $originalMeta = Get-Content -LiteralPath "docs/resultados/ej5_bt_original/${key}_medicion.json" -Raw -Encoding utf8 | ConvertFrom-Json
            $ascendingMeta = Get-Content -LiteralPath "docs/resultados/ej5_bt_ascendente/${key}_medicion.json" -Raw -Encoding utf8 | ConvertFrom-Json
            $yycMatrix = @($yycMethod.matrices | Where-Object { $_.archivo -eq "data/$key.txt" })
            if ($yycMatrix.Count -ne 1 -or $originalMeta.sha256 -ne $yycMatrix[0].sha256 -or
                $ascendingMeta.sha256_original -ne $yycMatrix[0].sha256 -or
                $ascendingMeta.sha256 -ne $yycMatrix[0].sha256_ascendente) {
                throw "Las mediciones no utilizan exactamente las mismas matrices: $key"
            }
            [pscustomobject]@{
                N=$row.N; matriz=$row.matriz; filas=$row.filas; columnas=$row.columnas
                testores_tipicos=$row.testores_tipicos
                yyc_original_ms=$row.yyc_original_ms; yyc_ascendente_ms=$row.yyc_ascendente_ms
                bt_original_ms=$(if ($original[0].estado -eq 'completado') { $original[0].bt_original_ms } else { 'ND' })
                bt_ascendente_ms=$(if ($ascending[0].estado -eq 'completado') { $ascending[0].bt_ascendente_ms } else { 'ND' })
            }
        })
        $table | Export-Csv -LiteralPath (Join-Path $Salida "tabla_${family}.csv") -NoTypeInformation -Encoding utf8
        $symbol = if ($family -eq 'phi') { '\varphi' } else { '\gamma' }
        $tex = @(
            '% Tablas 5 y 6 adaptadas: cinco columnas de matriz y cuatro tiempos.'
            '{\scriptsize'
            '\begin{tabular}{c l r r r r r r r}'
            '\toprule'
            '\multicolumn{5}{c}{} & \multicolumn{2}{c}{YYC (ms)} & \multicolumn{2}{c}{BT (ms)} \\'
            '\cmidrule(lr){6-7}\cmidrule(lr){8-9}'
            'N & Matriz & Filas & Cols. & $|\Psi^*|$ & Original & Ascendente & Original & Ascendente \\'
            '\midrule'
        )
        foreach ($row in $table) {
            $times = @('yyc_original_ms', 'yyc_ascendente_ms', 'bt_original_ms', 'bt_ascendente_ms') | ForEach-Object {
                if ($row.$_ -eq 'ND') { 'ND' } else {
                    [double]::Parse($row.$_, [cultureinfo]::InvariantCulture).ToString('F3', [cultureinfo]::InvariantCulture)
                }
            }
            $tex += ('{0} & ${1}^{{{0}}}(\theta(A,B))$ & {2} & {3} & {4} & {5} & {6} & {7} & {8} \\' -f
                $row.N, $symbol, $row.filas, $row.columnas, $row.testores_tipicos, $times[0], $times[1], $times[2], $times[3])
        }
        $tex += '\bottomrule', '\end{tabular}', '}'
        $tex += ('{\footnotesize Tiempos de una ejecución por orden y matriz, obtenidos en las sesiones registradas. Ordenamiento fuera de la medición. ND: proceso de BT interrumpido al alcanzar ' + $btOriginal[0].limite_proceso_s + ' s (original) o ' + $btAscending[0].limite_proceso_s + ' s (ascendente). El límite incluye preparación y verificación posterior; no es un tiempo del algoritmo. Los conteos de testores fueron comprobados con YYC.\par}')
        $tex | Set-Content -LiteralPath (Join-Path $Salida "tabla_${family}.tex") -Encoding utf8
    }
    Write-Host "Tablas con las cuatro columnas guardadas en $Salida"
}
finally { Pop-Location }
