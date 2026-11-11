param(
    [string]$Ejecutable = 'build-review/Release/testores.exe',
    [string]$Salida = '',
    [ValidateSet('original', 'ascendente')][string]$Orden = 'original',
    [ValidateRange(1, 86400)][int]$LimiteSegundos = 60,
    [switch]$Forzar
)

$ErrorActionPreference = 'Stop'
Push-Location -LiteralPath (Split-Path -Parent $PSScriptRoot)
try {
    if ([string]::IsNullOrWhiteSpace($Salida)) { $Salida = "docs/resultados/ej5_bt_$Orden" }
    $timeColumn = "bt_${Orden}_ms"
    $exePath = (Resolve-Path -LiteralPath $Ejecutable).Path
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
        $case.SourcePath = "data/$($case.Key).txt"
        $case.SourceHash = (Get-FileHash -LiteralPath $case.SourcePath -Algorithm SHA256).Hash
        $inputPath = if ($Orden -eq 'original') { $case.SourcePath } else {
            "docs/resultados/ej5_yyc_ascendente/matrices/$($case.Key)_ascendente.txt"
        }
        $case.InputPath = $inputPath
        $rows = @(Read-BinaryRows $inputPath)
        if ($rows.Count -ne $case.Rows) { throw "Cantidad de filas incorrecta: $inputPath" }
        foreach ($row in $rows) {
            if ($row.Length -ne $case.Cols -or $row -match '[^01]') { throw "Fila inválida: $inputPath" }
        }
        if ($Orden -eq 'ascendente') {
            $sourceRows = @(Read-BinaryRows $case.SourcePath)
            if ([string]::Join("`n", @($sourceRows | Sort-Object)) -cne
                [string]::Join("`n", @($rows | Sort-Object))) {
                throw "La matriz ordenada no conserva las filas: $inputPath"
            }
            $previous = -1
            foreach ($row in $rows) {
                $ones = $row.Length - $row.Replace('1', '').Length
                if ($ones -lt $previous) { throw "La matriz no está en orden ascendente: $inputPath" }
                $previous = $ones
            }
        }
        $case.Reference = "docs/resultados/ej5_yyc_ascendente/$($case.Key)_${Orden}_testores.txt"
        if (-not (Test-Path -LiteralPath $case.Reference -PathType Leaf)) {
            throw "Falta la referencia completa de YYC: $($case.Reference)"
        }
        $case.Hash = (Get-FileHash -LiteralPath $inputPath -Algorithm SHA256).Hash
    }
    New-Item -ItemType Directory -Path $Salida -Force | Out-Null
    $measurements = @{}

    foreach ($case in $cases) {
        Write-Host "BT ${Orden}: $($case.Key), límite del proceso $LimiteSegundos s"
        $prefix = Join-Path $Salida $case.Key
        $summaryPath = "${prefix}_resumen.tex"
        $setsPath = "${prefix}_testores.txt"
        # Evitar que una repetición interrumpida pueda reutilizar exportaciones antiguas.
        foreach ($oldExport in @($summaryPath, $setsPath)) {
            if (Test-Path -LiteralPath $oldExport) {
                if (-not $Forzar) { throw "Ya existe $oldExport" }
                Remove-Item -LiteralPath $oldExport
            }
        }
        $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
        $startInfo.FileName = $exePath
        $startInfo.WorkingDirectory = (Get-Location).Path
        $startInfo.UseShellExecute = $false
        $startInfo.CreateNoWindow = $true
        $startInfo.RedirectStandardOutput = $true
        $startInfo.RedirectStandardError = $true
        $startInfo.StandardOutputEncoding = [System.Text.Encoding]::UTF8
        $startInfo.StandardErrorEncoding = [System.Text.Encoding]::UTF8
        foreach ($argument in @('bt', '--entrada', $case.InputPath, '--latex', $summaryPath, '--testores-salida', $setsPath)) {
            $startInfo.ArgumentList.Add($argument)
        }
        if ($Forzar) { $startInfo.ArgumentList.Add('--forzar') }
        $process = [System.Diagnostics.Process]::new()
        $process.StartInfo = $startInfo
        $wallClock = [System.Diagnostics.Stopwatch]::StartNew()
        $timedOut = $false
        try {
            [void]$process.Start()
            $stdoutTask = $process.StandardOutput.ReadToEndAsync()
            $stderrTask = $process.StandardError.ReadToEndAsync()
            while (-not $process.WaitForExit(250)) {
                if ($wallClock.Elapsed.TotalSeconds -ge $LimiteSegundos) {
                    $timedOut = $true
                    if (-not $process.HasExited) { $process.Kill($true) }
                    $process.WaitForExit()
                    break
                }
            }
            $wallClock.Stop()
            $stdout = $stdoutTask.GetAwaiter().GetResult()
            $stderr = $stderrTask.GetAwaiter().GetResult()
            $exitCode = $process.ExitCode
        }
        finally {
            if (-not $process.HasExited) { $process.Kill($true); $process.WaitForExit() }
            $process.Dispose()
        }
        $stdout | Set-Content -LiteralPath "${prefix}_salida.txt" -Encoding utf8
        $stderr | Set-Content -LiteralPath "${prefix}_errores.txt" -Encoding utf8
        if ((Get-FileHash -LiteralPath $case.InputPath -Algorithm SHA256).Hash -ne $case.Hash -or
            (Get-FileHash -LiteralPath $case.SourcePath -Algorithm SHA256).Hash -ne $case.SourceHash) {
            throw "Una matriz cambió durante la medición: $($case.Key)"
        }

        $status = 'limite_proceso'
        $timeMs = ''
        $foundCount = $null
        $sameSets = $null
        if (-not $timedOut) {
            if ($exitCode -ne 0) { throw "Falló BT para $($case.Key): $stderr" }
            $match = [regex]::Match((Get-Content -LiteralPath $summaryPath -Raw -Encoding utf8), '(?m)^Tiempo total.* & ([0-9.]+) \\\\')
            if (-not $match.Success) { throw "No se encontró el tiempo de BT: $summaryPath" }
            $timeMs = ([double]::Parse($match.Groups[1].Value, [cultureinfo]::InvariantCulture) / 1000).ToString('F6', [cultureinfo]::InvariantCulture)
            $sets = @(Read-BinaryRows $setsPath)
            $reference = @(Read-BinaryRows $case.Reference)
            $foundCount = $sets.Count
            $sameSets = [string]::Join("`n", $sets) -ceq [string]::Join("`n", $reference)
            if ($foundCount -ne $case.Expected -or -not $sameSets) {
                throw "Los testores de BT no coinciden con YYC: $($case.Key)"
            }
            $status = 'completado'
            Write-Host "  $timeMs ms; $foundCount testores idénticos a YYC"
        }
        else { Write-Host '  Sin medición final: se alcanzó el límite del proceso.' }
        $measurements[$case.Key] = [pscustomobject]@{
            estado=$status; tiempo_ms=$timeMs
            tiempo_proceso_s=$wallClock.Elapsed.TotalSeconds.ToString('F3', [cultureinfo]::InvariantCulture)
        }
        [ordered]@{
            matriz=$case.InputPath; sha256=$case.Hash; estado=$status; orden=$Orden
            matriz_original=$case.SourcePath; sha256_original=$case.SourceHash
            limite_proceso_s=$LimiteSegundos; tiempo_proceso_s=$measurements[$case.Key].tiempo_proceso_s
            $timeColumn=$timeMs; testores_esperados=$case.Expected; testores_obtenidos=$foundCount
            coincide_con_YYC=$sameSets; codigo_salida=$exitCode
        } | ConvertTo-Json | Set-Content -LiteralPath "${prefix}_medicion.json" -Encoding utf8
    }

    foreach ($family in @('phi', 'gamma')) {
        $maximum = if ($family -eq 'phi') { 5 } else { 4 }
        $table = @(foreach ($n in 1..$maximum) {
            $key = if ($n -eq 1) { 'theta_AB' } else { "${family}${n}_theta_AB" }
            $case = $cases | Where-Object { $_.Key -eq $key }
            [pscustomobject]@{
                N=$n; matriz="${family}^$n(theta(A,B))"; filas=$case.Rows; columnas=$case.Cols
                testores_esperados=$case.Expected; $timeColumn=$measurements[$key].tiempo_ms
                estado=$measurements[$key].estado; limite_proceso_s=$LimiteSegundos
                tiempo_proceso_s=$measurements[$key].tiempo_proceso_s
            }
        })
        $table | Export-Csv -LiteralPath (Join-Path $Salida "tabla_${family}.csv") -NoTypeInformation -Encoding utf8
        $symbol = if ($family -eq 'phi') { '\varphi' } else { '\gamma' }
        $tex = @(
            "% BT en orden $Orden, una ejecución sin traza por matriz. N=1 compartido."
            '\begin{tabular}{c l r r r r l}'
            '\toprule'
            ('N & Matriz & Filas & Cols. & TT esperados & BT ' + $Orden + ' (ms) & Estado \\')
            '\midrule'
        )
        foreach ($entry in $table) {
            $value = if ($entry.estado -eq 'completado') { $entry.$timeColumn } else { 'ND' }
            $label = if ($entry.estado -eq 'completado') { 'Completo' } else { 'Límite' }
            $tex += ('{0} & ${1}^{{{0}}}(\theta(A,B))$ & {2} & {3} & {4} & {5} & {6} \\' -f
                $entry.N, $symbol, $entry.filas, $entry.columnas, $entry.testores_esperados, $value, $label)
        }
        $tex += '\bottomrule', '\end{tabular}'
        $tex += ('{{\footnotesize ND: el proceso no finalizó en {0} segundos. Este límite incluye preparación y verificación posterior; no es un tiempo medido del algoritmo.\par}}' -f $LimiteSegundos)
        $tex | Set-Content -LiteralPath (Join-Path $Salida "tabla_${family}.tex") -Encoding utf8
    }
    [ordered]@{
        fecha=(Get-Date).ToString('o'); algoritmo='BT'; orden=$Orden; ejecuciones_por_matriz=1
        limite_proceso_s=$LimiteSegundos
        medicion='Tiempo de BtSolver::solve exportado por el programa; excluye impresión, exportación y verificación posterior.'
        interrupciones='Límite de tiempo del proceso completo. Sin tiempo final ni resultado completo; no se interpreta como cota del tiempo de BtSolver::solve.'
        opciones='Opciones predeterminadas de BT: conserva hasta dos millones de testores, sin traza.'
        ejecutable_sha256=(Get-FileHash -LiteralPath $exePath -Algorithm SHA256).Hash
        verificacion='Para casos completos: igualdad exacta con todos los testores de YYC, y hash original sin cambios.'
        ordenamiento='Si ascendente: copias del punto 5.2.2, orden por cantidad de unos comprobado antes de medir; ordenamiento fuera del tiempo de BT.'
    } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $Salida 'metodologia.json') -Encoding utf8
    Write-Host "Tablas y evidencias guardadas en $Salida"
}
finally { Pop-Location }
