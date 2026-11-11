"""Prepara el código, las matrices y un cuaderno para medir BT en Colab."""

from __future__ import annotations

import json
import zipfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEST = ROOT / "colab"
NAME = "testores_bt_colab"
CASES = ("phi2", "phi4", "phi5", "gamma3", "gamma4")


def code_cell(source: str) -> dict:
    return {
        "cell_type": "code",
        "execution_count": None,
        "metadata": {},
        "outputs": [],
        "source": source.splitlines(keepends=True),
    }


def markdown_cell(source: str) -> dict:
    return {"cell_type": "markdown", "metadata": {}, "source": source.splitlines(keepends=True)}


def write_case_notebook(case: str, orders: tuple[str, ...] = ("original", "ascendente")) -> None:
    """Un cuaderno independiente para una matriz y los órdenes indicados."""
    title = {"phi2": "φ²", "phi4": "φ⁴", "phi5": "φ⁵", "gamma3": "γ³", "gamma4": "γ⁴"}[case]
    order_description = "solo el orden original" if orders == ("original",) else "la matriz original y luego la ascendente"
    cells = [
        markdown_cell(
            f"# BT para {title}(θ(A,B))\n\n"
            f"Ejecuta las tres celdas de código en orden. Este cuaderno mide {order_description}. "
            "Abre los otros "
            "cuadernos en pestañas separadas si deseas intentar varias matrices a la vez. "
            "Los resultados de Colab deben identificarse como tales.\n"
        ),
        code_cell(
            "from google.colab import files\n"
            "from pathlib import Path\n"
            "import zipfile\n"
            "uploaded = files.upload()  # Selecciona testores_bt_colab.zip\n"
            "assert 'testores_bt_colab.zip' in uploaded, 'Sube testores_bt_colab.zip'\n"
            "with zipfile.ZipFile('testores_bt_colab.zip') as z:\n"
            "    z.extractall('/content')\n"
            "ROOT = Path('/content/testores_bt_colab')\n"
            "assert (ROOT / 'CMakeLists.txt').is_file()\n"
            "print('Proyecto:', ROOT)\n"
        ),
        code_cell(
            "import subprocess\n"
            "subprocess.run(['cmake', '-S', str(ROOT), '-B', str(ROOT / 'build-colab'), "
            "'-DCMAKE_BUILD_TYPE=Release', '-DTESTORES_BUILD_TESTS=OFF'], check=True)\n"
            "subprocess.run(['cmake', '--build', str(ROOT / 'build-colab'), '--parallel', '2'], check=True)\n"
            "EXE = ROOT / 'build-colab/testores'\n"
            "assert EXE.is_file(), 'No se creó el ejecutable de Linux'\n"
            "print('Ejecutable:', EXE)\n"
        ),
        markdown_cell(
            f"## Medir {order_description}\n\n"
            "Ejecuta esta celda una vez. Muestra el tiempo y descarga un registro tras "
            "cada corrida. No hay límite de tiempo dentro del cuaderno; "
            "conserva cada registro descargado.\n"
        ),
        code_cell(
            "import csv, re, subprocess, time\n"
            "from google.colab import files\n"
            f"CASO = '{case}'\n"
            "results = ROOT / 'resultados'\n"
            "results.mkdir(exist_ok=True)\n"
            "rows = []\n"
            f"for order in {orders!r}:\n"
            "    name = f'{CASO}_theta_AB'\n"
            "    matrix = ROOT / 'data' / (f'{name}.txt' if order == 'original' "
            "else f'ascendente/{name}_ascendente.txt')\n"
            "    assert matrix.is_file(), matrix\n"
            "    log = results / f'bt_{CASO}_{order}.txt'\n"
            "    cmd = [str(EXE), 'bt', '--entrada', str(matrix)]\n"
            "    print(f'Iniciando {CASO} / {order}', flush=True)\n"
            "    started = time.monotonic()\n"
            "    with log.open('w', encoding='utf-8') as output:\n"
            "        process = subprocess.Popen(cmd, cwd=ROOT, stdout=output, stderr=subprocess.STDOUT)\n"
            "        try:\n"
            "            while process.poll() is None:\n"
            "                elapsed = time.monotonic() - started\n"
            "                print(f'  {elapsed / 60:.1f} min transcurridos', flush=True)\n"
            "                time.sleep(30)\n"
            "        except KeyboardInterrupt:\n"
            "            process.kill()\n"
            "            process.wait()\n"
            "            raise\n"
            "    output_text = log.read_text(encoding='utf-8', errors='replace')\n"
            "    match = re.search(r'Tiempo total de ejecuci[^\\n]*?:\\s*([0-9.]+)\\s*(µs|ms|s)', output_text)\n"
            "    if process.returncode == 0 and match:\n"
            "        value, unit = float(match.group(1)), match.group(2)\n"
            "        ms = value / 1000 if unit == 'µs' else value * 1000 if unit == 's' else value\n"
            "        status = 'completo'\n"
            "        print(f'{CASO} {order}: {value:.3f} {unit} = {ms:.3f} ms', flush=True)\n"
            "    else:\n"
            "        ms = ''\n"
            "        status = f'error_{process.returncode}'\n"
            "        print(f'{CASO} {order}: sin medición final ({status})', flush=True)\n"
            "    rows.append((CASO, order, status, ms, log.name))\n"
            "    summary = results / f'resumen_{CASO}.csv'\n"
            "    with summary.open('w', newline='', encoding='utf-8') as f:\n"
            "        writer = csv.writer(f)\n"
            "        writer.writerow(('matriz', 'orden', 'estado', 'tiempo_ms', 'registro'))\n"
            "        writer.writerows(rows)\n"
            "    files.download(str(log))\n"
            "files.download(str(summary))\n"
        ),
    ]
    notebook = {
        "cells": cells,
        "metadata": {"kernelspec": {"display_name": "Python 3", "language": "python", "name": "python3"},
                     "language_info": {"name": "python"}},
        "nbformat": 4,
        "nbformat_minor": 5,
    }
    suffix = "_original" if orders == ("original",) else ""
    target = DEST / f"medir_bt_{case}{suffix}.ipynb"
    target.write_text(json.dumps(notebook, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(target)


def main() -> None:
    DEST.mkdir(exist_ok=True)
    paths = [ROOT / "CMakeLists.txt", ROOT / "apps/main.cpp"]
    paths += sorted((ROOT / "include").rglob("*.hpp"))
    paths += sorted((ROOT / "src").rglob("*.cpp"))
    for case in CASES:
        base = f"{case}_theta_AB"
        paths += [
            ROOT / "data" / f"{base}.txt",
            ROOT / "docs/resultados/ej5_yyc_ascendente/matrices" / f"{base}_ascendente.txt",
        ]
    for path in paths:
        if not path.is_file():
            raise FileNotFoundError(path)

    with zipfile.ZipFile(DEST / f"{NAME}.zip", "w", zipfile.ZIP_DEFLATED) as archive:
        for path in paths:
            rel = path.relative_to(ROOT).as_posix()
            if rel.startswith("docs/resultados/ej5_yyc_ascendente/matrices/"):
                rel = "data/ascendente/" + path.name
            archive.write(path, f"{NAME}/{rel}")

    cells = [
        markdown_cell(
            "# Medición pendiente de BT en Google Colab\n\n"
            "Este cuaderno compila el mismo proyecto C++ para Linux y ejecuta **un caso por vez**. "
            "Los tiempos de Colab son de otra máquina y deben identificarse como tales en la tabla. "
            "Ejecuta las celdas en orden y conserva los archivos de resultados.\n"
        ),
        code_cell(
            "from google.colab import files\n"
            "from pathlib import Path\n"
            "import zipfile\n"
            "uploaded = files.upload()  # Selecciona testores_bt_colab.zip\n"
            "assert 'testores_bt_colab.zip' in uploaded, 'Sube testores_bt_colab.zip'\n"
            "with zipfile.ZipFile('testores_bt_colab.zip') as z:\n"
            "    z.extractall('/content')\n"
            "ROOT = Path('/content/testores_bt_colab')\n"
            "assert (ROOT / 'CMakeLists.txt').is_file()\n"
            "print('Proyecto extraído en:', ROOT)\n"
        ),
        code_cell(
            "import subprocess\n"
            "subprocess.run(['cmake', '-S', str(ROOT), '-B', str(ROOT / 'build-colab'), "
            "'-DCMAKE_BUILD_TYPE=Release', '-DTESTORES_BUILD_TESTS=OFF'], check=True)\n"
            "subprocess.run(['cmake', '--build', str(ROOT / 'build-colab'), '--parallel', '2'], check=True)\n"
            "EXE = ROOT / 'build-colab/testores'\n"
            "assert EXE.is_file(), 'No se creó el ejecutable de Linux'\n"
            "print('Ejecutable:', EXE)\n"
        ),
        markdown_cell(
            "## Ejecutar una medición\n\n"
            "Cambia `CASO` a `phi4`, `phi5`, `gamma3` o `gamma4`, y `ORDEN` a "
            "`original` o `ascendente`. Son ocho combinaciones. "
            "No ejecutes dos mediciones a la vez. La celda imprime un aviso cada 30 segundos. "
            "No hay límite de tiempo dentro del cuaderno. "
            "Un corte de sesión de Colab deja el caso pendiente.\n"
        ),
        code_cell(
            "import re, time, subprocess\n"
            "from google.colab import files\n"
            "CASO = 'phi4'          # phi4, phi5, gamma3 o gamma4\n"
            "ORDEN = 'original'    # original o ascendente\n"
            "assert CASO in ('phi4', 'phi5', 'gamma3', 'gamma4')\n"
            "assert ORDEN in ('original', 'ascendente')\n"
            "name = f'{CASO}_theta_AB'\n"
            "matrix = ROOT / 'data' / (f'{name}.txt' if ORDEN == 'original' "
            "else f'ascendente/{name}_ascendente.txt')\n"
            "assert matrix.is_file(), matrix\n"
            "results = ROOT / 'resultados'\n"
            "results.mkdir(exist_ok=True)\n"
            "log = results / f'bt_{CASO}_{ORDEN}.txt'\n"
            "cmd = [str(EXE), 'bt', '--entrada', str(matrix)]\n"
            "print('Ejecutando:', ' '.join(cmd), flush=True)\n"
            "print('Log:', log, flush=True)\n"
            "started = time.monotonic()\n"
            "with log.open('w', encoding='utf-8') as output:\n"
            "    process = subprocess.Popen(cmd, cwd=ROOT, stdout=output, stderr=subprocess.STDOUT)\n"
            "    try:\n"
            "        while process.poll() is None:\n"
            "            elapsed = time.monotonic() - started\n"
            "            print(f'Sigue ejecutándose: {elapsed / 60:.1f} min', flush=True)\n"
            "            time.sleep(30)\n"
            "    except KeyboardInterrupt:\n"
            "        process.kill()\n"
            "        process.wait()\n"
            "        print('Interrumpido: sin medición final.', flush=True)\n"
            "text = log.read_text(encoding='utf-8', errors='replace')\n"
            "match = re.search(r'Tiempo total de ejecuci[^\\n]*?:\\s*([0-9.]+)\\s*(µs|ms|s)', text)\n"
            "if process.returncode == 0 and match:\n"
            "    value, unit = float(match.group(1)), match.group(2)\n"
            "    ms = value / 1000 if unit == 'µs' else value * 1000 if unit == 's' else value\n"
            "    print(f'BT {CASO} {ORDEN}: {value:.3f} {unit} = {ms:.3f} ms')\n"
            "else:\n"
            "    print(f'Sin medición final (código {process.returncode}). Revisa el log.')\n"
            "files.download(str(log))\n"
        ),
        markdown_cell(
            "## Ejecutar las cuatro matrices automáticamente\n\n"
            "La siguiente celda ejecuta las **ocho combinaciones** (cuatro matrices, "
            "dos órdenes) de forma consecutiva. Usa esta opción **en vez de** repetir "
            "la celda individual. No hay límite de tiempo dentro del cuaderno. "
            "Al terminar descarga un ZIP con el resumen CSV y los registros. "
            "Si Colab corta la sesión antes, solo se conservan los resultados "
            "que hayas descargado previamente.\n"
        ),
        code_cell(
            "import csv, re, subprocess, time, zipfile\n"
            "from google.colab import files\n"
            "results = ROOT / 'resultados'\n"
            "results.mkdir(exist_ok=True)\n"
            "rows = []\n"
            "pairs = [(case, order) for case in ('phi4', 'phi5', 'gamma3', 'gamma4') "
            "for order in ('original', 'ascendente')]\n"
            "for case, order in pairs:\n"
            "    name = f'{case}_theta_AB'\n"
            "    matrix = ROOT / 'data' / (f'{name}.txt' if order == 'original' "
            "else f'ascendente/{name}_ascendente.txt')\n"
            "    assert matrix.is_file(), matrix\n"
            "    log = results / f'bt_{case}_{order}.txt'\n"
            "    cmd = [str(EXE), 'bt', '--entrada', str(matrix)]\n"
            "    print(f'Iniciando {case} / {order}', flush=True)\n"
            "    started = time.monotonic()\n"
            "    with log.open('w', encoding='utf-8') as output:\n"
            "        process = subprocess.Popen(cmd, cwd=ROOT, stdout=output, stderr=subprocess.STDOUT)\n"
            "        try:\n"
            "            while process.poll() is None:\n"
            "                elapsed = time.monotonic() - started\n"
            "                print(f'  {case} / {order}: {elapsed / 60:.1f} min', flush=True)\n"
            "                time.sleep(30)\n"
            "        except KeyboardInterrupt:\n"
            "            process.kill()\n"
            "            process.wait()\n"
            "            raise\n"
            "    output_text = log.read_text(encoding='utf-8', errors='replace')\n"
            "    match = re.search(r'Tiempo total de ejecuci[^\\n]*?:\\s*([0-9.]+)\\s*(µs|ms|s)', output_text)\n"
            "    if process.returncode == 0 and match:\n"
            "        value, unit = float(match.group(1)), match.group(2)\n"
            "        ms = value / 1000 if unit == 'µs' else value * 1000 if unit == 's' else value\n"
            "        status = 'completo'\n"
            "        print(f'  Terminado: {value:.3f} {unit} = {ms:.3f} ms', flush=True)\n"
            "    else:\n"
            "        ms = ''\n"
            "        status = f'error_{process.returncode}'\n"
            "        print(f'  Sin medición final: {status}', flush=True)\n"
            "    rows.append((case, order, status, ms, log.name))\n"
            "    with (results / 'resumen_bt_colab.csv').open('w', newline='', encoding='utf-8') as f:\n"
            "        writer = csv.writer(f)\n"
            "        writer.writerow(('matriz', 'orden', 'estado', 'tiempo_ms', 'registro'))\n"
            "        writer.writerows(rows)\n"
            "archive = ROOT / 'resultados_bt_colab.zip'\n"
            "with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as z:\n"
            "    for path in sorted(results.glob('*')):\n"
            "        z.write(path, path.name)\n"
            "print('Resumen:', results / 'resumen_bt_colab.csv')\n"
            "files.download(str(archive))\n"
        ),
    ]
    notebook = {
        "cells": cells,
        "metadata": {"kernelspec": {"display_name": "Python 3", "language": "python", "name": "python3"},
                     "language_info": {"name": "python"}},
        "nbformat": 4,
        "nbformat_minor": 5,
    }
    (DEST / "medir_bt.ipynb").write_text(json.dumps(notebook, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(DEST / f"{NAME}.zip")
    print(DEST / "medir_bt.ipynb")
    for case in ("phi2", "phi5", "gamma3", "gamma4"):
        write_case_notebook(case)
    write_case_notebook("phi4", ("original",))


if __name__ == "__main__":
    main()
