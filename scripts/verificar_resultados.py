#!/usr/bin/env python3
"""Oráculo independiente: enteros y definición de testor, sin usar el código C++."""
import csv
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "docs/resultados"


def read_matrix(path):
    rows = []
    width = None
    for line in path.read_text(encoding="utf-8-sig").splitlines():
        bits = re.sub(r"[\s,;]", "", line.split("#", 1)[0])
        if not bits:
            continue
        assert set(bits) <= {"0", "1"}, path
        width = len(bits) if width is None else width
        assert len(bits) == width, path
        rows.append(int(bits, 2))
    assert rows and width
    return width, rows


def typical(rows, mask):
    return all(mask & row for row in rows) and all(
        any((mask & row) == bit for row in rows)
        for bit in (1 << j for j in range(mask.bit_length()) if mask & (1 << j)))


def oracle(width, rows):
    assert width <= 20, "El oráculo exhaustivo se limita a matrices pequeñas"
    testors = [mask for mask in range(1 << width) if all(mask & row for row in rows)]
    return testors, {mask for mask in testors if typical(rows, mask)}


def basic(rows):
    return len(set(rows)) == len(rows) and not any(
        a != b and a & b == a for a in rows for b in rows)


def bt_steps(width, rows):
    alpha, steps = 1, []
    while alpha < 1 << width:
        uncovered = [(i, row) for i, row in enumerate(rows) if not alpha & row]
        if uncovered:
            index, row = max(uncovered, key=lambda item: (item[1] & -item[1], -item[0]))
            assert row  # estas matrices no tienen fila nula
            bit = row & -row
            next_alpha = (alpha & ~(bit - 1)) | bit
            steps.append((alpha, index + 1, next_alpha))
        else:
            next_alpha = alpha + (alpha & -alpha)
            steps.append((alpha, None, next_alpha))
        alpha = next_alpha
    return steps


def read_csv(name):
    with (OUT / name).open(encoding="utf-8-sig", newline="") as stream:
        return list(csv.DictReader(stream))


def parse_feature_sets(text, width):
    return {sum(1 << (width - int(x)) for x in re.findall(r"x(\d+)", group))
            for group in re.findall(r"\{([^}]+)\}", text)}


def main():
    width, a = read_matrix(ROOT / "data/matriz_A.txt")
    _, b = read_matrix(ROOT / "data/matriz_B.txt")
    assert width == 7 and len(a) == 6 and sum(row.bit_count() for row in a) == 20
    all_a, ta = oracle(width, a)
    all_b, tb = oracle(width, b)
    assert len(all_a) == 86 and len(ta) == 8
    assert len(all_b) == 42 and len(tb) == 6
    assert set(b) == ta and tb == set(a) and basic(a) and basic(b)
    records = read_csv("yyc_A.csv")
    assert len(records) == len(a)
    previous_time = 0.0
    for i, record in enumerate(records):
        expected = oracle(width, a[:i + 1])[1]
        assert parse_feature_sets(record["testores"], width) == expected
        assert int(record["num_testores"]) == len(expected)
        duration = float(record["tiempo_acumulado_us"])
        assert duration >= previous_time
        previous_time = duration
    steps = bt_steps(width, a)
    trace = (OUT / "bt_A_traza.txt").read_text(encoding="utf-8-sig")
    blocks = re.split(r"(?=Paso \d+:)", trace)[1:]
    assert len(steps) == len(blocks) == 44
    for block, (alpha, index, next_alpha) in zip(blocks, steps):
        assert int(re.search(r"α = ([01]{7})", block).group(1), 2) == alpha
        nxt = re.search(r"Siguiente: α' = ([01]{7})", block)
        assert (int(nxt.group(1), 2) if nxt else 1 << width) == next_alpha
        if index is not None:
            chosen = re.search(r"Se elige r(\d+)", block) or re.search(r"NO es testor: r(\d+)", block)
            assert int(chosen.group(1)) == index
        else:
            assert "ES testor" in block
    for name, rows, expected in (("A", a, ta), ("B", b, tb)):
        visited = [alpha for alpha, index, _ in bt_steps(width, rows) if index is None]
        found = read_csv(f"bt_{name}.csv")
        assert [int(row["alfa"], 2) for row in found] == visited
        assert {int(row["alfa"], 2) for row in found if row["tipico"] == "si"} == expected
        assert all((row["tipico"] == "si") == typical(rows, int(row["alfa"], 2)) for row in found)
    base = [(ar << width) | br for ar in a for br in b]
    base_tt = oracle(2 * width, base)[1]
    assert len(base_tt) == 14
    expected_base = {t << width for t in ta} | tb
    assert base_tt == expected_base
    for family, maximum in (("phi", 5), ("gamma", 4)):
        for n in range(1, maximum + 1):
            nwidth, generated = read_matrix(OUT / f"{family}_{n}.txt")
            if family == "phi":
                expected_rows = [sum(row << (2 * width * k) for k in range(n)) for row in base]
            else:
                expected_rows = [row << (2 * width * k) for k in reversed(range(n)) for row in base]
            assert generated == expected_rows and nwidth == 14 * n and basic(generated)
    experiments = read_csv("experimentos.csv")
    assert len(experiments) == 36
    states = {}
    for record in experiments:
        n = int(record["N"])
        count = 5 * n**2 + 7 * n**3 + 2 * n**4 if record["familia"] == "phi" else 14**n
        assert int(record["tipicos_esperados"]) == count
        assert record["estado"] in {"ok", "limite", "recurso"}
        assert bool(record["mediana_ns"]) == (record["estado"] == "ok")
        states[record["estado"]] = states.get(record["estado"], 0) + 1
    report = {"A": {"filas": 6, "columnas": 7, "unos": 20, "testores_totales": len(all_a),
                    "tipicos": len(ta), "candidatos_bt": len(steps), "testores_visitados_bt": 24},
              "B": {"testores_totales": len(all_b), "tipicos": len(tb), "dualidad_verificada": True},
              "yyc_estados_verificados": len(records), "bt_pasos_verificados": len(steps),
              "matrices_experimentales_verificadas": 9, "mediciones_experimentales": states,
              "metodo": "enumeración exhaustiva por definición con enteros; independiente del C++"}
    (OUT / "verificacion_independiente.json").write_text(
        json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, ensure_ascii=False))


if __name__ == "__main__":
    main()
