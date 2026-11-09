#include "testores/domain/testor.hpp"

#include <algorithm>
#include <stdexcept>

namespace testores::domain {

namespace {

void requireWidth(const BooleanMatrix& matrix, const ColumnSet& features) {
    if (features.width() != matrix.cols()) {
        throw std::invalid_argument("El conjunto de rasgos no corresponde al ancho de la matriz");
    }
}

// Columnas de T que tienen fila típica entre las primeras rowCount filas.
ColumnSet columnsWithTypicalRow(const BooleanMatrix& matrix, const ColumnSet& features, std::size_t rowCount) {
    requireWidth(matrix, features);
    const std::size_t limit = std::min(rowCount, matrix.rows());
    ColumnSet covered(matrix.cols());
    for (std::size_t i = 0; i < limit && covered != features; ++i) {
        const ColumnSet hit = matrix.row(i) & features;
        if (hit.count() == 1) {
            covered |= hit;
        }
    }
    return covered;
}

}  // namespace

bool isTestor(const BooleanMatrix& matrix, const ColumnSet& features) {
    requireWidth(matrix, features);
    return std::all_of(matrix.rowSets().begin(), matrix.rowSets().end(),
                       [&](const ColumnSet& r) { return r.intersects(features); });
}

bool isCompatibleSet(const BooleanMatrix& matrix, const ColumnSet& features, std::size_t rowCount) {
    return columnsWithTypicalRow(matrix, features, rowCount) == features;
}

std::optional<std::size_t> columnWithoutTypicalRow(const BooleanMatrix& matrix, const ColumnSet& features,
                                                   std::size_t rowCount) {
    const ColumnSet covered = columnsWithTypicalRow(matrix, features, rowCount);
    for (std::size_t col : features.indices()) {
        if (!covered.test(col)) {
            return col;
        }
    }
    return std::nullopt;
}

bool isTypicalTestor(const BooleanMatrix& matrix, const ColumnSet& features) {
    return features.any() && isTestor(matrix, features) && isCompatibleSet(matrix, features, matrix.rows());
}

}  // namespace testores::domain
