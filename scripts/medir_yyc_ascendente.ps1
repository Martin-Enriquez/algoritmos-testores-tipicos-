param(
    [string]$Ejecutable = 'build-review/Release/testores.exe',
    [string]$Salida = 'docs/resultados/ej5_yyc_ascendente',
    [switch]$Forzar
)

$ErrorActionPreference = 'Stop'
Push-Location -LiteralPath (Split-Path -Parent $PSScriptRoot)
try {
    if (-not (Test-Path -LiteralPath $Ejecutable -PathType Leaf)) { throw 'Compile primero el programa en Release.' }
    if ((Test-Path -LiteralPath $Salida) -and -not $Forzar) {
        throw "La carpeta $Salida ya existe. Use -Forzar para reemplazar estas mediciones."
    }
    $cases = @(
        @{ Key='theta_AB'; Rows=48; Cols=14; Expected=14 },
        @{ Key='phi2_theta_AB'; Rows=48; Cols=28; Expected=108 },
        @{ Key='phi3_theta_AB'; Rows=48; Cols=42; Expected=396 },
        @{ Key='phi4_theta_AB'; Rows=48; Cols=56; Expected=1040 },
        @{ Key='phi5_theta_AB'; Rows=48; Cols=70; Expected=2250 },
        @{ Key='gamma2_theta_AB'; Rows=96; Cols=28; Expected=196 },
        @{ Key='gamma3_theta_AB'; Rows=144; Cols=42; Expected=2744 },
        @{ Key='gamma4_theta_AB'; Rows=192; Cols=56; Expected=38416 }
    )

    function Read-BinaryRows([string]$Path) {
        Get-Content -LiteralPath $Path | ForEach-Object {
            $bits = (($_ -split '#', 2)[0] -replace '[\s,;]', '')
            if ($bits.Length -gt 0) { $bits }
        }
    }

    foreach ($case in $cases) {
        $inputPath = "data/$($case.Key).txt"
        $rows = @(Read-BinaryRows $inputPath)
        if ($rows.Count -ne $case.Rows) { throw "Cantidad de filas incorrecta: $inputPath" }
        foreach ($row in $rows) {
            if ($row.Length -ne $case.Cols -or $row -match '[^01]') { throw "Fila inválida: $inputPath" }
        }
        $case.Hash = (Get-FileHash -LiteralPath $inputPath -Algorithm SHA256).Hash
    }
    New-Item -ItemType Directory -Path (Join-Path $Salida 'matrices') -Force | Out-Null

    function Invoke-Measurement([string]$InputPath, [string]$Prefix, [int]$Rows, [int]$Expected) {
        $csvPath = Join-Path $Salida "${Prefix}_filas.csv"
        $setsPath = Join-Path $Salida "${Prefix}_testores.txt"
        $commandArgs = @('yyc', '--entrada', $InputPath, '--csv', $csvPath, '--testores-salida', $setsPath)
        if ($Forzar) { $commandArgs += '--forzar' }
        & $Ejecutable @commandArgs | Set-Content -LiteralPath (Join-Path $Salida "${Prefix}_salida.txt") -Encoding utf8
        if ($LASTEXITCODE -ne 0) { throw "Falló YYC: $InputPath" }
        $records = @(Import-Csv -LiteralPath $csvPath -Encoding utf8)
        if ($records.Count -ne $Rows -or [int]$records[-1].num_testores -ne $Expected) {
            throw "Cantidad de testores o filas incorrecta: $InputPath"
        }
        $sets = @(Read-BinaryRows $setsPath)
        if ($sets.Count -ne $Expected) { throw "La lista completa de testores está incompleta: $InputPath" }
        [pscustomobject]@{
            Ms = ([double]::Parse($records[-1].tiempo_acumulado_us, [cultureinfo]::InvariantCulture) / 1000)
            Sets = [string]::Join("`n", $sets)
        }
    }

    $measurements = @{}
    foreach ($case in $cases) {
        $inputPath = "data/$($case.Key).txt"
        $sortedPath = Join-Path $Salida "matrices/$($case.Key)_ascendente.txt"
        $sortArgs = @('ordenar', '--entrada', $inputPath, '--salida', $sortedPath)
        if ($Forzar) { $sortArgs += '--forzar' }
        & $Ejecutable @sortArgs | Set-Content -LiteralPath (Join-Path $Salida "$($case.Key)_ordenamiento.txt") -Encoding utf8
        if ($LASTEXITCODE -ne 0) { throw "Falló el ordenamiento: $inputPath" }

        $originalRows = @(Read-BinaryRows $inputPath)
        $sortedRows = @(Read-BinaryRows $sortedPath)
        if ([string]::Join("`n", @($originalRows | Sort-Object)) -cne
            [string]::Join("`n", @($sortedRows | Sort-Object))) {
            throw "El ordenamiento cambió el contenido de las filas: $inputPath"
        }
        $previous = -1
        foreach ($row in $sortedRows) {
            $ones = $row.Length - $row.Replace('1', '').Length
            if ($ones -lt $previous) { throw "Las filas no están en orden ascendente: $sortedPath" }
            $previous = $ones
        }

        Write-Host "YYC: $($case.Key), referencia original y orden ascendente"
        # Repetir la referencia con el mismo ejecutable y en la misma sesión.
        # No se reemplazan las evidencias previas del punto 5.2.1.
        $original = Invoke-Measurement $inputPath "$($case.Key)_original" $case.Rows $case.Expected
        $ascending = Invoke-Measurement $sortedPath "$($case.Key)_ascendente" $case.Rows $case.Expected
        if ($original.Sets -cne $ascending.Sets) { throw "Los conjuntos de testores no coinciden: $inputPath" }
        if ((Get-FileHash -LiteralPath $inputPath -Algorithm SHA256).Hash -ne $case.Hash) {
            throw "Se modificó la matriz original: $inputPath"
        }
        $case.SortedHash = (Get-FileHash -LiteralPath $sortedPath -Algorithm SHA256).Hash
        $measurements[$case.Key] = @{
            Original=$original.Ms.ToString('F6', [cultureinfo]::InvariantCulture)
            Ascending=$ascending.Ms.ToString('F6', [cultureinfo]::InvariantCulture)
        }
        Write-Host "  Original: $($measurements[$case.Key].Original) ms; ascendente: $($measurements[$case.Key].Ascending) ms; $($case.Expected) conjuntos idénticos"
    }

    foreach ($family in @('phi', 'gamma')) {
        $maximum = if ($family -eq 'phi') { 5 } else { 4 }
        $table = @(foreach ($n in 1..$maximum) {
            $key = if ($n -eq 1) { 'theta_AB' } else { "${family}${n}_theta_AB" }
            $case = $cases | Where-Object { $_.Key -eq $key }
            [pscustomobject]@{
                N=$n; matriz="${family}^$n(theta(A,B))"; filas=$case.Rows; columnas=$case.Cols
                testores_tipicos=$case.Expected
                yyc_original_ms=$measurements[$key].Original
                yyc_ascendente_ms=$measurements[$key].Ascending
            }
        })
        $table | Export-Csv -LiteralPath (Join-Path $Salida "tabla_${family}.csv") -NoTypeInformation -Encoding utf8
        $symbol = if ($family -eq 'phi') { '\varphi' } else { '\gamma' }
        $tex = @(
            '% Una ejecución por orden y matriz, sin traza. Tiempos en ms.'
            '% Ordenamiento estable fuera de la medición. N=1 se comparte en ambas tablas.'
            '\begin{tabular}{c l r r r r r}'
            '\toprule'
            'N & Matriz & Filas & Cols. & $|\Psi^*|$ & YYC orig. (ms) & YYC asc. (ms) \\'
            '\midrule'
        )
        foreach ($entry in $table) {
            $tex += ('{0} & ${1}^{{{0}}}(\theta(A,B))$ & {2} & {3} & {4} & {5} & {6} \\' -f
                $entry.N, $symbol, $entry.filas, $entry.columnas, $entry.testores_tipicos, $entry.yyc_original_ms, $entry.yyc_ascendente_ms)
        }
        $tex += '\bottomrule', '\end{tabular}'
        $tex | Set-Content -LiteralPath (Join-Path $Salida "tabla_${family}.tex") -Encoding utf8
    }
    [ordered]@{
        fecha=(Get-Date).ToString('o'); algoritmo='YYC'; ejecuciones_por_orden_y_matriz=1
        medicion='Suma de intervalos de processNextRow, sin impresión, exportación ni ordenamiento.'
        ordenamiento='Ascendente por número de unos; empates conservan el orden original.'
        referencia='Original seguido de ascendente, mismo ejecutable y sesión; N=1 compartido entre tablas.'
        verificacion='Igualdad exacta de todos los testores típicos en orden canónico; filas conservadas y originales sin cambios.'
        ejecutable_sha256=(Get-FileHash -LiteralPath $Ejecutable -Algorithm SHA256).Hash
        matrices=@($cases | ForEach-Object {
            [ordered]@{ archivo="data/$($_.Key).txt"; sha256=$_.Hash; sha256_ascendente=$_.SortedHash; testores=$_.Expected }
        })
    } | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $Salida 'metodologia.json') -Encoding utf8
    Write-Host "Tablas y evidencias guardadas en $Salida"
}
finally { Pop-Location }
