#include "testores/presentation/console_view.hpp"

#include <algorithm>
#include <iomanip>
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
    std::size_t zero = 0;
    std::size_t duplicate = 0;
    std::size_t superrow = 0;
    out << "\nDetalle de la reducción a matriz básica:\n";
    for (std::size_t i = 0; i < reduction.decisions.size(); ++i) {
        const auto& decision = reduction.decisions[i];
        switch (decision.fate) {
            case domain::RowFate::Kept:
                continue;
            case domain::RowFate::ZeroRow:
                ++zero;
                out << "  " << rowLabel(i) << " eliminada: fila nula\n";
                break;
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
    if (zero + duplicate + superrow == 0) {
        out << "  (ninguna fila eliminada: la matriz ya era básica)\n";
    }
    out << "\nResumen:\n"
        << "  Filas originales:           " << original.rows() << '\n'
        << "  Filas nulas eliminadas:     " << zero << '\n'
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
