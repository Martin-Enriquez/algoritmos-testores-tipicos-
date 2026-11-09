#include "testores/domain/bt.hpp"

#include <algorithm>
#include <string>
#include <utility>

#include "testores/domain/testor.hpp"

namespace testores::domain {

namespace {

// Última columna (mayor índice) presente en el conjunto. Requiere conjunto no vacío.
std::size_t lastColumn(const ColumnSet& set) {
    const std::size_t last = set.lastIndex();
    if (last == set.width()) {
        throw std::logic_error("BT: se pidió el último 1 de un conjunto vacío");
    }
    return last;
}

// Primera fila de la matriz con ceros en todas las columnas de α.
std::optional<std::size_t> firstZeroRow(const BooleanMatrix& matrix, const ColumnSet& alpha) {
    for (std::size_t i = 0; i < matrix.rows(); ++i) {
        if (!matrix.row(i).intersects(alpha)) {
            return i;
        }
    }
    return std::nullopt;
}

}  // namespace

std::optional<std::uint64_t> btNumber(const ColumnSet& alpha) {
    const std::size_t n = alpha.width();
    if (n > 63) {
        return std::nullopt;
    }
    std::uint64_t value = 0;
    for (std::size_t col = 0; col < n; ++col) {
        value = (value << 1U) | (alpha.test(col) ? 1U : 0U);
    }
    return value;
}

ColumnSet btJumpNotTestor(const ColumnSet& alpha, std::size_t pivot) {
    if (alpha.test(pivot)) {
        throw std::invalid_argument("BT: la columna del salto ya pertenece a α");
    }
    ColumnSet next = alpha;
    next.set(pivot);
    for (std::size_t col = pivot + 1; col < next.width(); ++col) {
        next.set(col, false);
    }
    return next;
}

std::optional<ColumnSet> btJumpTestor(const ColumnSet& alpha) {
    ColumnSet next = alpha;
    // Sumar 1 en la posición del último 1: esa posición y los unos contiguos
    // a su izquierda pasan a 0 y el primer 0 que se encuentra pasa a 1.
    std::size_t col = lastColumn(alpha);
    while (true) {
        if (!next.test(col)) {
            next.set(col);
            return next;
        }
        next.set(col, false);
        if (col == 0) {
            return std::nullopt;  // desbordamiento: no hay más n-uplos
        }
        --col;
    }
}

BtResult BtSolver::solve(const BooleanMatrix& matrix, const BtOptions& options) {
    if (matrix.empty()) {
        throw std::invalid_argument("BT requiere una matriz con al menos una fila y una columna");
    }
    for (const ColumnSet& row : matrix.rowSets()) {
        if (row.none()) {
            throw std::invalid_argument("BT requiere una matriz sin filas nulas (use su matriz básica)");
        }
    }

    BtResult result;
    const std::size_t n = matrix.cols();
    ColumnSet start(n);
    start.set(n - 1);  // α = (0, ..., 0, 1)
    std::optional<ColumnSet> alpha = start;

    while (alpha) {
        ++result.stats.candidatesEvaluated;
        BtStep step;
        const std::optional<std::size_t> zeroRow = firstZeroRow(matrix, *alpha);
        std::optional<ColumnSet> next;

        if (zeroRow) {
            // (S1) α no es testor: saltar al primer n-uplo que cubre la fila de ceros.
            step.verdict = BtVerdict::NotTestor;
            step.zeroRow = *zeroRow;
            step.pivot = lastColumn(matrix.row(*zeroRow));
            next = btJumpNotTestor(*alpha, step.pivot);
        } else {
            // (S2) α es testor: decidir tipicidad y saltar sus superconjuntos consecutivos.
            ++result.stats.testorsFound;
            const bool typical = isCompatibleSet(matrix, *alpha, matrix.rows());
            step.verdict = typical ? BtVerdict::TypicalTestor : BtVerdict::NonTypicalTestor;
            if (!typical && options.recordTrace) {
                step.columnWithoutTypicalRow = columnWithoutTypicalRow(matrix, *alpha, matrix.rows());
            }
            step.pivot = lastColumn(*alpha);
            if (typical) {
                ++result.stats.typicalFound;
                if (result.typicalTestors.size() >= options.maxTypicalTestors) {
                    throw BtResourceLimitExceeded("BT superó el límite de " +
                                                  std::to_string(options.maxTypicalTestors) + " testores típicos");
                }
                result.typicalTestors.push_back(*alpha);
            }
            if (result.testors.size() < options.maxStoredTestors) {
                result.testors.push_back({*alpha, typical});
            } else {
                result.testorsTruncated = true;
            }
            next = btJumpTestor(*alpha);
        }

        if (options.recordTrace) {
            if (result.trace.size() < options.maxTraceSteps) {
                step.candidate = *alpha;
                step.next = next;
                result.trace.push_back(std::move(step));
            } else {
                result.traceTruncated = true;
            }
        }
        alpha = std::move(next);
    }

    std::sort(result.typicalTestors.begin(), result.typicalTestors.end(), canonicalLess);
    return result;
}

std::vector<ColumnSet> minimalSets(const std::vector<ColumnSet>& sets) {
    std::vector<ColumnSet> sorted = sets;
    std::sort(sorted.begin(), sorted.end(), canonicalLess);
    sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());
    // En orden canónico (por cardinalidad) un subconjunto propio siempre aparece
    // antes, así que basta comparar con los minimales ya aceptados.
    std::vector<ColumnSet> minimal;
    for (const ColumnSet& candidate : sorted) {
        const bool dominated = std::any_of(minimal.begin(), minimal.end(),
                                           [&](const ColumnSet& m) { return m.isSubsetOf(candidate); });
        if (!dominated) {
            minimal.push_back(candidate);
        }
    }
    return minimal;
}

}  // namespace testores::domain
