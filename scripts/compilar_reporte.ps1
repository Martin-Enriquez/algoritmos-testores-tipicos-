param([string]$PdfLatex = 'pdflatex')
$ErrorActionPreference = 'Stop'
Push-Location (Join-Path (Split-Path -Parent $PSScriptRoot) 'docs')
try {
    for ($pass = 0; $pass -lt 2; $pass++) {
        & $PdfLatex -interaction=nonstopmode -halt-on-error reporte.tex
        if ($LASTEXITCODE -ne 0) { throw 'Falló la compilación del informe; consulte docs/reporte.log.' }
    }
    Write-Output 'Informe generado en docs/reporte.pdf.'
} finally { Pop-Location }
