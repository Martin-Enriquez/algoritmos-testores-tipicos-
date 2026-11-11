#include "testores/domain/matrix_operators.hpp"

#include <stdexcept>
#include <algorithm>
#include <utility>
#include <vector>

namespace testores::domain {

BooleanMatrix theta(const BooleanMatrix& a, const BooleanMatrix& b) {
    if (a.empty() || b.empty()) {
        throw std::invalid_argument("Theta requiere dos matrices no vacías");
    }
    if (a.cols() > MatrixLimits::kMaxCols - b.cols()) {
        throw std::invalid_argument("Theta excede el máximo de columnas permitido");
    }
    // Comprobar antes de multiplicar y reservar memoria.
    if (a.rows() > MatrixLimits::kMaxRows / b.rows()) {
        throw std::invalid_argument("Theta excede el máximo de filas permitido");
    }

    const std::size_t cols = a.cols() + b.cols();
    std::vector<ColumnSet> rows;
    rows.reserve(a.rows() * b.rows());
    for (const ColumnSet& left : a.rowSets()) {
        for (const ColumnSet& right : b.rowSets()) {
            ColumnSet joined(cols);
            for (std::size_t col : left.indices()) {
                joined.set(col);
            }
            for (std::size_t col : right.indices()) {
                joined.set(a.cols() + col);
            }
            rows.push_back(std::move(joined));
        }
    }
    return BooleanMatrix(cols, std::move(rows));
}

BooleanMatrix phi(const BooleanMatrix& a, const BooleanMatrix& b) {
    if (a.empty() || b.empty() || a.rows() != b.rows()) {
        throw std::invalid_argument("Phi requiere dos matrices no vacías con el mismo número de filas");
    }
    if (a.cols() > MatrixLimits::kMaxCols - b.cols()) {
        throw std::invalid_argument("Phi excede el máximo de columnas permitido");
    }
    const std::size_t cols = a.cols() + b.cols();
    std::vector<ColumnSet> rows;
    rows.reserve(a.rows());
    for (std::size_t i = 0; i < a.rows(); ++i) {
        ColumnSet joined(cols);
        for (std::size_t col : a.row(i).indices()) joined.set(col);
        for (std::size_t col : b.row(i).indices()) joined.set(a.cols() + col);
        rows.push_back(std::move(joined));
    }
    return BooleanMatrix(cols, std::move(rows));
}

BooleanMatrix phiPower(const BooleanMatrix& a, std::size_t copies) {
    if (a.empty() || copies == 0) {
        throw std::invalid_argument("Phi requiere una matriz no vacía y al menos una copia");
    }
    if (copies > MatrixLimits::kMaxCols / a.cols()) {
        throw std::invalid_argument("Phi excede el máximo de columnas permitido");
    }
    const std::size_t cols = a.cols() * copies;
    std::vector<ColumnSet> rows;
    rows.reserve(a.rows());
    for (const ColumnSet& row : a.rowSets()) {
        ColumnSet joined(cols);
        const auto indices = row.indices();
        for (std::size_t copy = 0; copy < copies; ++copy) {
            for (std::size_t col : indices) joined.set(copy * a.cols() + col);
        }
        rows.push_back(std::move(joined));
    }
    return BooleanMatrix(cols, std::move(rows));
}

BooleanMatrix gamma(const BooleanMatrix& a, const BooleanMatrix& b) {
    if (a.empty() || b.empty()) {
        throw std::invalid_argument("Gamma requiere dos matrices no vacías");
    }
    if (a.cols() > MatrixLimits::kMaxCols - b.cols() ||
        a.rows() > MatrixLimits::kMaxRows - b.rows()) {
        throw std::invalid_argument("Gamma excede las dimensiones máximas permitidas");
    }
    const std::size_t cols = a.cols() + b.cols();
    std::vector<ColumnSet> rows;
    rows.reserve(a.rows() + b.rows());
    for (const ColumnSet& row : a.rowSets()) {
        ColumnSet padded(cols);
        for (std::size_t col : row.indices()) padded.set(col);
        rows.push_back(std::move(padded));
    }
    for (const ColumnSet& row : b.rowSets()) {
        ColumnSet padded(cols);
        for (std::size_t col : row.indices()) padded.set(a.cols() + col);
        rows.push_back(std::move(padded));
    }
    return BooleanMatrix(cols, std::move(rows));
}

BooleanMatrix gammaPower(const BooleanMatrix& a, std::size_t copies) {
    if (a.empty() || copies == 0) {
        throw std::invalid_argument("Gamma requiere una matriz no vacía y al menos una copia");
    }
    if (copies > MatrixLimits::kMaxCols / a.cols() ||
        copies > MatrixLimits::kMaxRows / a.rows()) {
        throw std::invalid_argument("Gamma excede las dimensiones máximas permitidas");
    }
    const std::size_t cols = a.cols() * copies;
    std::vector<ColumnSet> rows;
    rows.reserve(a.rows() * copies);
    for (std::size_t copy = 0; copy < copies; ++copy) {
        for (const ColumnSet& row : a.rowSets()) {
            ColumnSet padded(cols);
            for (std::size_t col : row.indices()) padded.set(copy * a.cols() + col);
            rows.push_back(std::move(padded));
        }
    }
    return BooleanMatrix(cols, std::move(rows));
}

BooleanMatrix sortRowsByOnes(const BooleanMatrix& matrix) {
    auto rows = matrix.rowSets();
    std::stable_sort(rows.begin(), rows.end(), [](const ColumnSet& a, const ColumnSet& b) {
        return a.count() < b.count();
    });
    return BooleanMatrix(matrix.cols(), std::move(rows));
}

}  // namespace testores::domain
