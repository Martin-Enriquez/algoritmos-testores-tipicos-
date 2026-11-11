param(
    [string]$Ejecutable = 'build-review/Release/testores.exe',
    [string]$Salida = 'docs/resultados/ej5_yyc_original',
    [switch]$Forzar
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
Push-Location -LiteralPath $projectRoot
try {
    if (-not (Test-Path -LiteralPath $Ejecutable -PathType Leaf)) {
        throw "No existe el ejecutable: $Ejecutable. Compile primero en Release."
    }
    if ((Test-Path -LiteralPath $Salida) -and -not $Forzar) {
        throw "La carpeta de resultados ya existe: $Salida. Use -Forzar para repetir las mediciones."
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

    # Validar todas las entradas antes de iniciar la primera medición.
    foreach ($case in $cases) {
        $inputPath = "data/$($case.Key).txt"
        if (-not (Test-Path -LiteralPath $inputPath -PathType Leaf)) {
            throw "Falta la matriz: $inputPath"
        }
        $rows = @(Get-Content -LiteralPath $inputPath | ForEach-Object {
            $line = ($_ -split '#', 2)[0].Trim()
            if ($line.Length -gt 0) { $line }
        })
        if ($rows.Count -ne $case.Rows) { throw "Cantidad de filas incorrecta: $inputPath" }
        foreach ($row in $rows) {
            $bits = $row -replace '[\s,;]', ''
            if ($bits.Length -ne $case.Cols -or $bits -match '[^01]') {
                throw "Fila inválida en $inputPath"
            }
        }
        $case.Hash = (Get-FileHash -LiteralPath $inputPath -Algorithm SHA256).Hash
    }

    New-Item -ItemType Directory -Path $Salida -Force | Out-Null
    $measurements = @{}
    foreach ($case in $cases) {
        Write-Host "Midiendo YYC, orden original: $($case.Key) ($($case.Rows) x $($case.Cols))"
        $csvPath = Join-Path $Salida "$($case.Key)_filas.csv"
        $logPath = Join-Path $Salida "$($case.Key)_salida.txt"
        $commandArgs = @('yyc', '--entrada', "data/$($case.Key).txt", '--csv', $csvPath)
        if ($Forzar) { $commandArgs += '--forzar' }
        & $Ejecutable @commandArgs | Set-Content -LiteralPath $logPath -Encoding utf8
        if ($LASTEXITCODE -ne 0) { throw "Falló YYC para $($case.Key)" }
        $records = @(Import-Csv -LiteralPath $csvPath -Encoding utf8)
        if ($records.Count -ne $case.Rows -or [int]$records[-1].num_testores -ne $case.Expected) {
            throw "El resultado no coincide con la predicción para $($case.Key)"
        }
        if ((Get-FileHash -LiteralPath "data/$($case.Key).txt" -Algorithm SHA256).Hash -ne $case.Hash) {
            throw "La matriz cambió durante la medición: $($case.Key)"
        }
        $us = [double]::Parse($records[-1].tiempo_acumulado_us, [cultureinfo]::InvariantCulture)
        $measurements[$case.Key] = [pscustomobject]@{
            filas = $case.Rows; columnas = $case.Cols; testores_tipicos = $case.Expected
            yyc_original_ms = ($us / 1000).ToString('F6', [cultureinfo]::InvariantCulture)
        }
        Write-Host "  $($case.Expected) testores típicos; $($measurements[$case.Key].yyc_original_ms) ms"
    }

    foreach ($family in @('phi', 'gamma')) {
        $maximum = if ($family -eq 'phi') { 5 } else { 4 }
        $table = @(foreach ($n in 1..$maximum) {
            $key = if ($n -eq 1) { 'theta_AB' } else { "${family}${n}_theta_AB" }
            $measurement = $measurements[$key]
            [pscustomobject]@{
                N = $n; matriz = "${family}^$n(theta(A,B))"
                filas = $measurement.filas; columnas = $measurement.columnas
                testores_tipicos = $measurement.testores_tipicos
                yyc_original_ms = $measurement.yyc_original_ms
            }
        })
        $table | Export-Csv -LiteralPath (Join-Path $Salida "tabla_${family}.csv") -NoTypeInformation -Encoding utf8
        $symbol = if ($family -eq 'phi') { '\varphi' } else { '\gamma' }
        $tex = @(
            '% YYC: orden original, una ejecución por matriz, sin traza. Tiempos en ms.'
            '% N=1 reutiliza la misma medición de theta(A,B) en ambas tablas.'
            '\begin{tabular}{c l r r r r}'
            '\toprule'
            'N & Matriz & Filas & Columnas & $|\Psi^*|$ & YYC original (ms) \\'
            '\midrule'
        )
        foreach ($entry in $table) {
            $tex += ('{0} & ${1}^{{{0}}}(\theta(A,B))$ & {2} & {3} & {4} & {5} \\' -f
                $entry.N, $symbol, $entry.filas, $entry.columnas, $entry.testores_tipicos, $entry.yyc_original_ms)
        }
        $tex += '\bottomrule', '\end{tabular}'
        $tex | Set-Content -LiteralPath (Join-Path $Salida "tabla_${family}.tex") -Encoding utf8
    }

    [ordered]@{
        fecha = (Get-Date).ToString('o')
        algoritmo = 'YYC'; orden = 'original'; ejecuciones_por_matriz = 1
        medicion = 'Suma de los intervalos de processNextRow; excluye impresión y exportación. Sin traza.'
        caso_N1 = 'La misma ejecución de theta_AB se utiliza en ambas tablas.'
        ejecutable = (Resolve-Path -LiteralPath $Ejecutable).Path
        ejecutable_sha256 = (Get-FileHash -LiteralPath $Ejecutable -Algorithm SHA256).Hash
        matrices = @($cases | ForEach-Object {
            [ordered]@{ archivo="data/$($_.Key).txt"; sha256=$_.Hash; testores_esperados=$_.Expected }
        })
    } | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $Salida 'metodologia.json') -Encoding utf8
    Write-Host "Tablas y evidencias guardadas en $Salida"
}
finally {
    Pop-Location
}
