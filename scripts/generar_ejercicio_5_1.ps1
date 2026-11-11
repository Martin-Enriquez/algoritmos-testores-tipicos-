param(
    [string]$Ejecutable = 'build-review/Release/testores.exe',
    [string]$SalidaMatrices = 'data/ej5_1',
    [string]$SalidaResultados = 'docs/resultados/ej5_1',
    [switch]$Forzar
)

$ErrorActionPreference = 'Stop'
Push-Location -LiteralPath (Split-Path -Parent $PSScriptRoot)
try {
    if (-not (Test-Path -LiteralPath $Ejecutable -PathType Leaf)) { throw 'Compile primero el programa en Release.' }
    foreach ($folder in @($SalidaMatrices, $SalidaResultados)) {
        if ((Test-Path -LiteralPath $folder) -and -not $Forzar) {
            throw "Ya existe $folder. Use otra carpeta de salida o -Forzar para reemplazar la generación."
        }
    }
    foreach ($folder in @($SalidaMatrices, $SalidaResultados)) {
        New-Item -ItemType Directory -Path $folder -Force | Out-Null
    }
    function Read-Rows([string]$Path) {
        Get-Content -LiteralPath $Path | ForEach-Object {
            $row = (($_ -split '#', 2)[0] -replace '[\s,;]', '')
            if ($row.Length -gt 0) { $row }
        }
    }
    function Invoke-Testores([string[]]$CommandArgs, [string]$LogName) {
        & $Ejecutable @CommandArgs | Set-Content -LiteralPath (Join-Path $SalidaResultados $LogName) -Encoding utf8
        if ($LASTEXITCODE -ne 0) { throw "Falló testores: $($CommandArgs -join ' ')" }
    }
    $overwrite = @()
    if ($Forzar) { $overwrite += '--forzar' }
    $rowsA = @(Read-Rows 'data/matriz_A.txt')
    $rowsB = @(Read-Rows 'data/matriz_B.txt')
    $inputHashes = @{
        A=(Get-FileHash -LiteralPath 'data/matriz_A.txt' -Algorithm SHA256).Hash
        B=(Get-FileHash -LiteralPath 'data/matriz_B.txt' -Algorithm SHA256).Hash
    }
    $basePath = Join-Path $SalidaMatrices 'theta_AB.txt'
    Invoke-Testores (@('theta', '--entrada-a', 'data/matriz_A.txt', '--entrada-b', 'data/matriz_B.txt', '--salida', $basePath) + $overwrite) 'theta_AB_generacion.txt'
    $baseRows = @(Read-Rows $basePath)
    $expectedBase = @(foreach ($aRow in $rowsA) { foreach ($bRow in $rowsB) { $aRow + $bRow } })
    if ([string]::Join("`n", $baseRows) -cne [string]::Join("`n", $expectedBase)) {
        throw 'Theta no coincide con todas las concatenaciones A_i|B_j en el orden previsto.'
    }
    $cols = $baseRows[0].Length

    # Sólo se resuelven A y B, pequeñas. Los conteos de las familias se deducen
    # de las proposiciones del artículo, no se presentan como ejecuciones de BT.
    $baseLengths = @()
    foreach ($name in @('A', 'B')) {
        $setPath = Join-Path $SalidaResultados "testores_${name}.txt"
        Invoke-Testores (@('yyc', '--entrada', "data/matriz_$name.txt", '--testores-salida', $setPath) + $overwrite) "yyc_${name}.txt"
        $sets = @(Read-Rows $setPath)
        foreach ($set in $sets) { $baseLengths += $set.Length - $set.Replace('1', '').Length }
    }
    $baseCount = $baseLengths.Count
    $tables = @{}
    $evidence = @()
    foreach ($family in @('phi', 'gamma')) {
        $maximum = if ($family -eq 'phi') { 5 } else { 4 }
        $tables[$family] = @(foreach ($n in 1..$maximum) {
            $path = $basePath
            if ($n -gt 1) {
                $path = Join-Path $SalidaMatrices "${family}${n}_theta_AB.txt"
                Invoke-Testores (@($family, '--entrada', $basePath, '--copias', "$n", '--salida', $path) + $overwrite) "${family}${n}_generacion.txt"
            }
            $expected = if ($family -eq 'phi') {
                @(foreach ($row in $baseRows) { $row * $n })
            } else {
                @(foreach ($copy in 0..($n - 1)) {
                    foreach ($row in $baseRows) { ('0' * ($copy * $cols)) + $row + ('0' * (($n - $copy - 1) * $cols)) }
                })
            }
            $actual = @(Read-Rows $path)
            if ([string]::Join("`n", $actual) -cne [string]::Join("`n", $expected)) {
                throw "Contenido u orden de filas incorrecto: $path"
            }
            $basicLog = "${family}${n}_basica.txt"
            Invoke-Testores @('basica', '--entrada', $path) $basicLog
            if ((Get-Content -LiteralPath (Join-Path $SalidaResultados $basicLog) -Raw) -notmatch 'ninguna fila eliminada') {
                throw "La matriz no es básica: $path"
            }
            $count = [long]0
            if ($family -eq 'phi') {
                foreach ($length in $baseLengths) { $count += [long][math]::Pow($n, $length) }
            } else { $count = [long][math]::Pow($baseCount, $n) }
            $evidence += [ordered]@{
                familia=$family; N=$n; archivo=$path; sha256=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
                filas=$actual.Count; columnas=$actual[0].Length; es_basica=$true; testores_teoricos=$count
            }
            Write-Host "$family N=$n : $($actual.Count) x $($actual[0].Length), $count testores teóricos; básica verificada"
            [pscustomobject]@{ N=$n; matriz="${family}^$n(theta(A,B))"; filas=$actual.Count; columnas=$actual[0].Length; testores_teoricos=$count }
        })
        $tables[$family] | Export-Csv -LiteralPath (Join-Path $SalidaResultados "tabla_${family}.csv") -NoTypeInformation -Encoding utf8
        $symbol = if ($family -eq 'phi') { '\varphi' } else { '\gamma' }
        $tex = @(
            '% Generado por generar_ejercicio_5_1.ps1: conteos teóricos, no tiempos.'
            '\begin{tabular}{c l r r r}'
            '\toprule'
            '$N$ & Matriz & Filas & Columnas & $|\Psi^*|$ teórico \\'
            '\midrule'
        )
        foreach ($entry in $tables[$family]) {
            $tex += ('{0} & ${1}^{{{0}}}(C)$ & {2} & {3} & {4} \\' -f
                $entry.N, $symbol, $entry.filas, $entry.columnas, $entry.testores_teoricos)
        }
        $tex += '\bottomrule', '\end{tabular}'
        $tex | Set-Content -LiteralPath (Join-Path $SalidaResultados "tabla_${family}.tex") -Encoding utf8
    }
    if ((Get-FileHash -LiteralPath 'data/matriz_A.txt' -Algorithm SHA256).Hash -ne $inputHashes.A -or
        (Get-FileHash -LiteralPath 'data/matriz_B.txt' -Algorithm SHA256).Hash -ne $inputHashes.B) {
        throw 'Las matrices A o B cambiaron durante la generación.'
    }
    [ordered]@{
        entradas=@{ A='data/matriz_A.txt'; B='data/matriz_B.txt'; hashes=$inputHashes }
        columnas_base=$cols; filas_base=$baseRows.Count; testores_base_teoricos=$baseCount
        longitudes_testores_base=$baseLengths; resultados=$evidence
        observacion='Los conteos se calculan analíticamente. N=1 reutiliza C en ambas familias. No se reordenan filas.'
    } | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $SalidaResultados 'generacion.json') -Encoding utf8
    Write-Host "Matrices: $SalidaMatrices; tablas y verificación: $SalidaResultados"
}
finally { Pop-Location }
