#include "testores/presentation/console_view.hpp"

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <optional>
#include <ostream>
#include <sstream>

#include "testores/domain/notation.hpp"

namespace testores::presentation {

using domain::BooleanMatrix;
using domain::ColumnSet;
using domain::formatFeatureSet;
using domain::formatFeatureSets;

namespace {

std::string rowLabel(std::size_t index) { return "r" + std::to_string(index + 1); }

std::string rowAsBits(const ColumnSet& row) {
    std::string out;
    for (std::size_t j = 0; j < row.width(); ++j) {
        out += (j > 0 ? " " : "");
        out += row.test(j) ? '1' : '0';
    }
    return out;
}

std::string percent(double value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(4) << value << " (" << std::setprecision(2) << value * 100.0 << " %)";
    return out.str();
}

}  // namespace

std::string formatDuration(std::chrono::nanoseconds ns) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(3);
    const double value = static_cast<double>(ns.count());
    if (ns < std::chrono::microseconds(1000)) {
        out << value / 1e3 << " µs";
    } else if (ns < std::chrono::milliseconds(1000)) {
        out << value / 1e6 << " ms";
    } else {
        out << value / 1e9 << " s";
    }
    return out.str();
}

void printMatrix(std::ostream& out, const BooleanMatrix& matrix, const std::string& title) {
    out << "\n--- " << title << " (" << matrix.rows() << " x " << matrix.cols() << ") ---\n";
    if (matrix.rows() == 0) {
        out << "(matriz vacía)\n";
        return;
    }
    const std::size_t labelWidth = rowLabel(matrix.rows() - 1).size() + 1;
    const std::size_t cellWidth = domain::featureName(matrix.cols() - 1).size() + 1;

    out << std::string(labelWidth + 2, ' ');
    for (std::size_t j = 0; j < matrix.cols(); ++j) {
        out << std::setw(static_cast<int>(cellWidth)) << domain::featureName(j);
    }
    out << '\n';
    for (std::size_t i = 0; i < matrix.rows(); ++i) {
        out << std::left << std::setw(static_cast<int>(labelWidth)) << rowLabel(i) << std::right << "| ";
        for (std::size_t j = 0; j < matrix.cols(); ++j) {
            out << std::setw(static_cast<int>(cellWidth)) << (matrix.at(i, j) ? 1 : 0);
        }
        out << '\n';
    }
}

void printReduction(std::ostream& out, const BooleanMatrix& original, const domain::BasicReduction& reduction) {
    std::size_t duplicate = 0;
    std::size_t superrow = 0;
    out << "\nDetalle de la reducción a matriz básica:\n";
    if (const auto nullRow = domain::firstNullRow(original)) {
        out << "  " << rowLabel(*nullRow)
            << " es nula: es subfila de toda fila no nula, así que la matriz básica es una sola fila nula.\n"
               "  Ningún conjunto de rasgos cubre una fila nula: esta matriz NO tiene testores.\n";
    }
    for (std::size_t i = 0; i < reduction.decisions.size(); ++i) {
        const auto& decision = reduction.decisions[i];
        switch (decision.fate) {
            case domain::RowFate::Kept:
                continue;
            case domain::RowFate::Duplicate:
                ++duplicate;
                out << "  " << rowLabel(i) << " eliminada: repetida de " << rowLabel(decision.witness) << '\n';
                break;
            case domain::RowFate::Superrow:
                ++superrow;
                out << "  " << rowLabel(i) << " eliminada: superfila de " << rowLabel(decision.witness) << " ("
                    << rowAsBits(original.row(decision.witness)) << " es subfila de " << rowAsBits(original.row(i))
                    << ")\n";
                break;
        }
    }
    if (duplicate + superrow == 0) {
        out << "  (ninguna fila eliminada: la matriz ya era básica)\n";
    }
    out << "\nResumen:\n"
        << "  Filas originales:           " << original.rows() << '\n'
        << "  Filas repetidas eliminadas: " << duplicate << '\n'
        << "  Superfilas eliminadas:      " << superrow << '\n'
        << "  Filas de la matriz básica:  " << reduction.basic.rows() << '\n'
        << "  Densidad original:          " << percent(original.density()) << '\n'
        << "  Densidad matriz básica:     " << percent(reduction.basic.density()) << '\n';
}

std::string formatFeatureSetsLimited(const std::vector<ColumnSet>& sets, std::size_t limit) {
    if (sets.size() <= limit) {
        std::vector<ColumnSet> sorted = sets;
        std::sort(sorted.begin(), sorted.end(), domain::canonicalLess);
        return formatFeatureSets(sorted);
    }
    std::vector<ColumnSet> first(limit);
    std::partial_sort_copy(sets.begin(), sets.end(), first.begin(), first.end(), domain::canonicalLess);
    return formatFeatureSets(first) + ", ... y " + std::to_string(sets.size() - limit) + " más";
}

void printYycSummaryTable(std::ostream& out, const application::YycRunResult& result, std::size_t limit) {
    out << "\nTabla de resultados YYC";
    if (result.repetitions > 1) {
        out << " (tiempos: mediana de " << result.repetitions << " ejecuciones)";
    }
    out << ":\n";
    out << std::left << std::setw(6) << "Fila" << std::setw(9) << "|Ψ*|" << std::setw(16) << "T. acumulado"
        << "Testores típicos de las filas procesadas\n";
    for (const auto& row : result.rows) {
        out << std::left << std::setw(6) << rowLabel(row.rowIndex) << std::setw(8) << row.testorCount
            << std::setw(16) << formatDuration(row.cumulative)
            << (row.testorsOmitted ? std::string("(omitidos por tamaño)")
                                   : formatFeatureSetsLimited(row.testors, limit))
            << '\n';
    }
    out << std::right;
    out << "\nTestores típicos de la matriz (" << result.typicalTestors.size()
        << "): " << formatFeatureSetsLimited(result.typicalTestors, limit) << '\n'
        << "Tiempo total de ejecución: " << formatDuration(result.total) << '\n';
}

namespace {

std::string tuple(const ColumnSet& alpha) {
    return domain::toBinaryString(alpha) + " = " + formatFeatureSet(alpha);
}

// Cantidad de n-uplos estrictamente entre `from` y `to` (vacío si n > 63).
std::optional<std::uint64_t> skippedBetween(const ColumnSet& from, const ColumnSet& to) {
    const auto a = domain::btNumber(from);
    const auto b = domain::btNumber(to);
    if (!a || !b || *b <= *a) {
        return std::nullopt;
    }
    return *b - *a - 1;
}

std::optional<std::uint64_t> skippedUntilEnd(const ColumnSet& from) {
    const auto a = domain::btNumber(from);
    if (!a) {
        return std::nullopt;
    }
    const std::uint64_t last = (std::uint64_t{1} << from.width()) - 1;
    return last - *a;
}

}  // namespace

void printBtTrace(std::ostream& out, const BooleanMatrix& matrix, const domain::BtResult& result) {
    const std::size_t n = matrix.cols();
    out << "\nRecorrido de BT: los n-uplos α = (a1 ... a" << n
        << ") se leen como números binarios (a1 = bit más significativo)\n"
           "y se analizan en orden creciente, con saltos, desde "
        << std::string(n - 1, '0') << "1 hasta que el acarreo indica el fin.\n";
    for (std::size_t s = 0; s < result.trace.size(); ++s) {
        const domain::BtStep& step = result.trace[s];
        out << "\nPaso " << (s + 1) << ": α = " << tuple(step.candidate) << '\n';
        std::optional<std::uint64_t> skipped;
        switch (step.verdict) {
            case domain::BtVerdict::NotTestor: {
                std::vector<std::size_t> uncovered;
                for (std::size_t i = 0; i < matrix.rows(); ++i) {
                    if (!matrix.row(i).intersects(step.candidate)) {
                        uncovered.push_back(i);
                    }
                }
                const std::string chosen =
                    rowLabel(step.zeroRow) + " = " + domain::toBinaryString(matrix.row(step.zeroRow));
                if (step.zeroRowIsNull) {
                    out << "  NO es testor: " << chosen << " es una fila nula; ningún n-uplo la cubre.\n"
                        << "  La matriz no tiene testores: FIN del recorrido.\n";
                    break;
                }
                if (uncovered.size() == 1) {
                    out << "  NO es testor: " << chosen << " es la única fila con ceros en todas las columnas de α.\n";
                } else {
                    out << "  NO es testor: filas con ceros en todas las columnas de α:";
                    for (std::size_t k = 0; k < uncovered.size(); ++k) {
                        const std::size_t i = uncovered[k];
                        out << (k == 0 ? " " : ", ") << rowLabel(i) << " (último 1 en "
                            << domain::featureName(matrix.row(i).lastIndex()) << ')';
                    }
                    out << ".\n  Se elige " << chosen << ", la de último 1 más a la izquierda (salto más grande).\n";
                }
                out << "  Salto S1: último 1 de " << rowLabel(step.zeroRow) << " en " << domain::featureName(step.pivot)
                    << " → poner 1 en " << domain::featureName(step.pivot) << " y ceros a su derecha.\n";
                break;
            }
            case domain::BtVerdict::TypicalTestor:
            case domain::BtVerdict::NonTypicalTestor:
                out << "  ES testor (toda fila tiene un 1 en alguna columna de α) y ";
                if (step.verdict == domain::BtVerdict::TypicalTestor) {
                    out << "es TÍPICO: cada rasgo tiene fila típica.\n";
                } else {
                    out << "NO es típico: " << domain::featureName(step.columnWithoutTypicalRow.value_or(0))
                        << " no tiene fila típica (se puede quitar y sigue siendo testor).\n";
                }
                out << "  Salto S2: último 1 de α en " << domain::featureName(step.pivot) << " → sumar 1 en esa posición"
                    << " (se omiten los superconjuntos de α que sólo agregan rasgos a la derecha).\n";
                break;
        }
        if (step.next) {
            skipped = skippedBetween(step.candidate, *step.next);
            out << "  Siguiente: α' = " << tuple(*step.next);
        } else if (step.zeroRowIsNull) {
            continue;
        } else {
            skipped = skippedUntilEnd(step.candidate);
            out << "  El acarreo se sale por la izquierda: FIN del recorrido";
        }
        if (skipped && *skipped > 0) {
            out << (*skipped == 1 ? "  (se salta 1 n-uplo)" : "  (se saltan " + std::to_string(*skipped) + " n-uplos)");
        }
        out << '\n';
    }
    if (result.traceTruncated) {
        out << "\n(La traza se recortó: hubo más pasos de los que se guardan.)\n";
    }
}

void printBtSummary(std::ostream& out, const BooleanMatrix& matrix, const application::BtRunResult& result,
                    std::size_t limit) {
    const domain::BtResult& run = result.run;
    out << "\nResultados de BT:\n  n-uplos analizados: " << run.stats.candidatesEvaluated;
    if (matrix.cols() <= 63) {
        const std::uint64_t total = (std::uint64_t{1} << matrix.cols()) - 1;
        out << " de " << total << " posibles (2^" << matrix.cols() << " - 1); se saltaron "
            << (total - run.stats.candidatesEvaluated);
    }
    out << "\n  Testores encontrados en los candidatos visitados: " << run.stats.testorsFound
        << " (típicos: " << run.stats.typicalFound
        << ", no típicos: " << (run.stats.testorsFound - run.stats.typicalFound) << ")\n";
    out << "  Los saltos omiten otros testores no típicos; este conteo no es el total de testores de la matriz.\n";

    if (run.testors.size() <= limit) {
        out << "\n  #    α" << std::string(matrix.cols() > 1 ? matrix.cols() - 1 : 0, ' ')
            << "   ¿Típico?  Testor\n";
        for (std::size_t i = 0; i < run.testors.size(); ++i) {
            const auto& testor = run.testors[i];
            out << "  " << std::left << std::setw(4) << (i + 1) << ' ' << domain::toBinaryString(testor.set) << "   "
                << (testor.typical ? "sí" : "no") << "         " << formatFeatureSet(testor.set) << std::right
                << '\n';
        }
    } else {
        out << "  (lista de testores omitida: " << run.testors.size() << " > " << limit
            << "; use --completo para verla)\n";
    }
    if (run.testorsTruncated) {
        out << "  (sólo se guardaron los primeros " << run.testors.size() << " testores)\n";
    }

    out << "\nTestores típicos de la matriz (" << run.typicalTestors.size()
        << "): " << formatFeatureSetsLimited(run.typicalTestors, limit) << '\n';
    out << "Identificación de la tipicidad:\n"
           "  - durante la ejecución: testor cuyos rasgos tienen todos fila típica;\n"
           "  - a posteriori: testores minimales (por inclusión) entre los encontrados.\n";
    if (run.testorsTruncated) {
        out << "  Comprobación a posteriori no disponible (lista de testores recortada).\n";
    } else {
        out << "  Ambos procedimientos " << (result.typicalityProceduresAgree ? "coinciden ✓" : "NO coinciden ✗")
            << '\n';
    }
    out << "Tiempo total de ejecución: " << formatDuration(result.total);
    if (result.repetitions > 1) {
        out << " (mediana de " << result.repetitions << " ejecuciones)";
    }
    out << '\n';
}

void ConsoleYycListener::onRowProcessed(const application::YycRowProgress& progress) {
    if (explain_) {
        explainStep(progress);
    }
    out_ << "Tras procesar " << rowLabel(progress.rowIndex) << " (" << (progress.rowIndex + 1) << "/"
         << progress.totalRows << "): |Ψ*| = "
         << progress.current.size() << " | tiempo acumulado = " << formatDuration(progress.cumulative) << '\n'
         << "  Ψ* = " << formatFeatureSetsLimited(progress.current, limit_) << '\n';
}

void ConsoleYycListener::explainStep(const application::YycRowProgress& progress) {
    const std::size_t k = progress.rowIndex;
    const ColumnSet& row = matrix_.row(k);
    out_ << "\n=== Paso " << (k + 1) << ": fila " << rowLabel(k) << " = (" << rowAsBits(row)
         << "), columnas con 1: " << formatFeatureSet(row) << " ===\n";

    if (k == 0) {
        out_ << "  Inicialización: Ψ* = { {xj} : r1[j] = 1 }.\n";
        return;
    }
    std::vector<ColumnSet> previous;
    for (const auto& decision : progress.trace.decisions) {
        previous.push_back(decision.tau);
    }
    out_ << "  Ψ* anterior (en orden de recorrido): " << formatFeatureSets(previous) << '\n';

    for (const auto& decision : progress.trace.decisions) {
        if (decision.coversRow) {
            out_ << "  τ = " << formatFeatureSet(decision.tau) << " ya cubre " << rowLabel(k) << " (τ ∩ "
                 << rowLabel(k) << " = " << formatFeatureSet(decision.tau & row) << ") → se conserva.\n";
            continue;
        }
        out_ << "  τ = " << formatFeatureSet(decision.tau) << " no cubre " << rowLabel(k)
             << " → se prueba τ ∪ {x} para cada x ∈ " << formatFeatureSet(row) << ":\n";
        for (const auto& candidate : decision.extensions) {
            ColumnSet extended = decision.tau;
            extended.set(candidate.added);
            out_ << "      " << formatFeatureSet(extended);
            if (candidate.accepted) {
                out_ << " es compatible → se agrega.\n";
            } else {
                out_ << " NO es compatible";
                if (candidate.columnWithoutTypicalRow) {
                    out_ << " (" << domain::featureName(*candidate.columnWithoutTypicalRow)
                         << " se queda sin fila típica)";
                }
                out_ << " → se descarta.\n";
            }
        }
    }
}

}  // namespace testores::presentation
