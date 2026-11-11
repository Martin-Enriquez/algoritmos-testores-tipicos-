#!/usr/bin/env python3
"""Regenera el entorno y las macros del reporte a partir de resultados reales."""
import argparse
import csv
import json
import math
import os
import platform
import re
from pathlib import Path


def latex(value):
    replacements = {"\\": r"\textbackslash{}", "_": r"\_", "%": r"\%", "&": r"\&",
                    "#": r"\#", "$": r"\$", "{": r"\{", "}": r"\}",
                    "~": r"\textasciitilde{}", "^": r"\textasciicircum{}"}
    return "".join(replacements.get(char, char) for char in str(value))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", default="build")
    parser.add_argument("--repeticiones", type=int, default=9)
    parser.add_argument("--limite-ms", type=int, default=10000)
    args = parser.parse_args()
    if not 1 <= args.repeticiones <= 10000:
        parser.error("--repeticiones debe estar en [1, 10000]")
    if not 1 <= args.limite_ms <= 3600000:
        parser.error("--limite-ms debe estar en [1, 3600000]")
    root = Path(__file__).resolve().parents[1]
    out = root / "docs" / "resultados"
    cache = (root / args.build / "CMakeCache.txt").read_text(encoding="utf-8")
    mode = re.search(r"^CMAKE_BUILD_TYPE:STRING=(.*)$", cache, re.M)
    build_mode = mode.group(1) if mode and mode.group(1) else "Release (multiconfiguración)"
    if build_mode != "Release" and not build_mode.startswith("Release ("):
        raise SystemExit("El informe de rendimiento requiere compilación Release")
    compiler = "C++17"
    compiler_files = sorted((root / args.build / "CMakeFiles").glob("*/CMakeCXXCompiler.cmake"),
                            key=lambda path: path.stat().st_mtime, reverse=True)
    if compiler_files:
        text = compiler_files[0].read_text(encoding="utf-8")
        identity = re.search(r'set\(CMAKE_CXX_COMPILER_ID "([^"]*)"\)', text)
        version = re.search(r'set\(CMAKE_CXX_COMPILER_VERSION "([^"]*)"\)', text)
        if identity and version:
            compiler = identity.group(1) + " " + version.group(1) + ", C++17"
    cpu = platform.processor() or "no informado por el sistema"
    if os.name == "nt":
        try:
            import winreg
            with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE,
                                r"HARDWARE\DESCRIPTION\System\CentralProcessor\0") as key:
                cpu = winreg.QueryValueEx(key, "ProcessorNameString")[0].strip()
        except OSError:
            pass
    elif Path("/proc/cpuinfo").exists():
        match = re.search(r"model name\s*:\s*(.+)", Path("/proc/cpuinfo").read_text())
        if match:
            cpu = match.group(1)
    count = sum(len(re.findall(r"^TEST\(", p.read_text(encoding="utf-8"), re.M))
                for p in (root / "tests").glob("test_*.cpp"))
    with (out / "yyc_A.csv").open(encoding="utf-8-sig", newline="") as stream:
        yyc_rows = list(csv.DictReader(stream))
    if not yyc_rows:
        raise SystemExit("El CSV de YYC está vacío; regenere los resultados")
    yyc_time = yyc_rows[-1]["tiempo_acumulado_us"]
    bt_summary = (out / "bt_A_resumen.tex").read_text(encoding="utf-8-sig")
    bt_match = re.search(r"Tiempo total.*?&\s*([\d.]+)", bt_summary)
    if not bt_match:
        raise SystemExit("No se encontró el tiempo total en el resumen de BT")
    bt_time = bt_match.group(1)
    if any(not math.isfinite(float(value)) or float(value) < 0 for value in (yyc_time, bt_time)):
        raise SystemExit("Los tiempos del informe deben ser números finitos no negativos")
    experiment_csv = out / "experimentos.csv"
    if experiment_csv.exists():
        with experiment_csv.open(encoding="utf-8-sig", newline="") as stream:
            for row in csv.DictReader(stream):
                if row["estado"] == "ok" and int(row["repeticiones"]) != args.repeticiones:
                    raise SystemExit("Las repeticiones solicitadas no coinciden con los experimentos guardados")
    macros = (f"% Generado a partir del código y los CSV del proyecto.\n"
              f"\\newcommand{{\\NumeroPruebas}}{{{count}}}\n"
              f"\\newcommand{{\\TiempoYycA}}{{{yyc_time}}}\n"
              f"\\newcommand{{\\TiempoBtA}}{{{bt_time}}}\n"
              f"\\newcommand{{\\RepeticionesExperimento}}{{{args.repeticiones}}}\n"
              f"\\newcommand{{\\LimiteExperimento}}{{{args.limite_ms / 1000:g}}}\n")
    (out / "resultados_resumen.tex").write_text(macros, encoding="utf-8")
    system = f"{platform.system()} {platform.release()} ({platform.machine()})"
    paragraph = ("Las mediciones incluidas se obtuvieron en " + latex(system) + ", con procesador "
                 + latex(cpu) + ", compilador " + latex(compiler) + " y configuración "
                 + latex(build_mode) + ". Se utilizó un reloj monótono de alta resolución. "
                 "Las tablas de A y B usan 10\\,000 ejecuciones; las tablas de operadores usan "
                 "\\RepeticionesExperimento{} ejecuciones medidas precedidas de un calentamiento. "
                 "Los tiempos dependen de la carga del equipo y no son constantes matemáticas.\n")
    (out / "entorno_experimental.tex").write_text(paragraph, encoding="utf-8")
    metadata = dict(system=system, cpu=cpu, compiler=compiler, build=build_mode,
                    unit_tests=count, algorithm_repetitions=10000,
                    experiment_repetitions=args.repeticiones,
                    experiment_warmups=1, experiment_limit_ms=args.limite_ms,
                    time_unit="nanoseconds in experiment CSV, milliseconds in experiment tables")
    (out / "entorno.json").write_text(json.dumps(metadata, ensure_ascii=False, indent=2) + "\n",
                                       encoding="utf-8")
    print("Metadatos del informe actualizados")


if __name__ == "__main__":
    main()
