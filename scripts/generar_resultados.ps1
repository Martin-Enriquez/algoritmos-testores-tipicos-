param(
    [string]$BuildDirectory = 'build',
    [ValidateRange(1,10000)][int]$Repeticiones = 9,
    [ValidateRange(1,3600000)][int]$LimiteMs = 10000,
    [string]$Python = 'python'
)
$ErrorActionPreference = 'Stop'
if (-not (Get-Command -Name $Python -ErrorAction SilentlyContinue)) {
    throw 'No se encontró Python. Indique su ejecutable con -Python ruta/al/python.exe.'
}
$previousConsoleEncoding = [Console]::OutputEncoding
$previousOutputEncoding = $OutputEncoding
[Console]::OutputEncoding = [Text.UTF8Encoding]::new($false)
$OutputEncoding = [Console]::OutputEncoding
$projectRoot = Split-Path -Parent $PSScriptRoot
Push-Location $projectRoot
try {
    $program = Join-Path $BuildDirectory 'testores.exe'
    $experiments = Join-Path $BuildDirectory 'testores_experimentos.exe'
    if (-not (Test-Path -LiteralPath $program)) { $program = Join-Path $BuildDirectory 'Release/testores.exe' }
    if (-not (Test-Path -LiteralPath $experiments)) { $experiments = Join-Path $BuildDirectory 'Release/testores_experimentos.exe' }
    if (-not (Test-Path -LiteralPath $program) -or -not (Test-Path -LiteralPath $experiments)) {
        throw 'Compile el proyecto en Release antes de generar los resultados.'
    }
    $program = (Resolve-Path -LiteralPath $program).Path
    $experiments = (Resolve-Path -LiteralPath $experiments).Path
    $out = 'docs/resultados'
    New-Item -ItemType Directory -Force -Path $out | Out-Null
    function Invoke-Testores([string[]]$NativeArguments, [string]$Log) {
        & $program @NativeArguments | Set-Content -LiteralPath $Log -Encoding UTF8
        if ($LASTEXITCODE -ne 0) { throw "Falló testores: $($NativeArguments -join ' ')" }
    }
    Invoke-Testores -NativeArguments @('generar','--filas','100','--columnas','10','--semilla','2026','--salida','data/ej1_aleatoria_100x10.txt','--salida-basica','data/ej1_basica_100x10.txt','--forzar') -Log "$out/ej1_100x10.txt"
    Invoke-Testores -NativeArguments @('generar','--filas','8','--columnas','7','--semilla','2026','--salida','data/matriz_A_original.txt','--forzar') -Log "$out/ej2_original.txt"
    Invoke-Testores -NativeArguments @('buscar','--columnas','7','--filas','8','--densidad-min','0.4','--min-filas-basica','6','--max-filas-basica','6','--semilla','2026','--salida','data/matriz_A.txt','--forzar') -Log "$out/ej2_1_busqueda_A.txt"
    Invoke-Testores -NativeArguments @('yyc','--entrada','data/matriz_A.txt','--traza','--completo') -Log "$out/yyc_A_traza.txt"
    Invoke-Testores -NativeArguments @('yyc','--entrada','data/matriz_A.txt','--repeticiones','10000','--completo','--csv',"$out/yyc_A.csv",'--latex',"$out/yyc_A_tabla.tex",'--latex-matriz',"$out/matriz_A.tex",'--forzar') -Log "$out/yyc_A_tabla.txt"
    Invoke-Testores -NativeArguments @('bt','--entrada','data/matriz_A.txt','--traza','--completo','--latex-traza',"$out/bt_A_traza.tex",'--forzar') -Log "$out/bt_A_traza.txt"
    Invoke-Testores -NativeArguments @('bt','--entrada','data/matriz_A.txt','--repeticiones','10000','--completo','--csv',"$out/bt_A.csv",'--latex',"$out/bt_A_resumen.tex",'--forzar') -Log "$out/bt_A.txt"
    Set-Content -LiteralPath "$out/experimentos_log.txt" -Value @() -Encoding UTF8
    & $experiments --repeticiones $Repeticiones --limite-ms $LimiteMs --salida $out --forzar | ForEach-Object {
        Add-Content -LiteralPath "$out/experimentos_log.txt" -Value $_ -Encoding UTF8
        Write-Output $_
    }
    if ($LASTEXITCODE -ne 0) { throw 'Falló la generación de experimentos.' }
    Copy-Item -LiteralPath "$out/matriz_B.txt" -Destination 'data/matriz_B.txt' -Force
    Invoke-Testores -NativeArguments @('bt','--entrada','data/matriz_B.txt','--repeticiones','10000','--completo','--csv',"$out/bt_B.csv",'--latex',"$out/bt_B_resumen.tex",'--forzar') -Log "$out/bt_B.txt"
    Invoke-Testores -NativeArguments @('yyc','--entrada','data/matriz_B.txt','--latex-matriz',"$out/matriz_B.tex",'--forzar') -Log "$out/yyc_B.txt"
    Invoke-Testores -NativeArguments @('bt','--entrada','data/matriz_B.txt','--traza','--completo') -Log "$out/bt_B_traza.txt"
    & $Python scripts/actualizar_metadatos.py --build $BuildDirectory --repeticiones $Repeticiones --limite-ms $LimiteMs
    if ($LASTEXITCODE -ne 0) { throw 'Falló la generación de metadatos.' }
    Write-Output 'Resultados y estados de ejecución guardados en docs/resultados.'
} finally {
    Pop-Location
    [Console]::OutputEncoding = $previousConsoleEncoding
    $OutputEncoding = $previousOutputEncoding
}
