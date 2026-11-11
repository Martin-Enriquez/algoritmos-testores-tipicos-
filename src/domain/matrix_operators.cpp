#include "testores/domain/matrix_operators.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace testores::domain {
namespace {
void nonempty(const BooleanMatrix& matrix) {
    if (matrix.empty()) throw std::invalid_argument("El operador requiere matrices no vacías");
}
void dimensions(std::size_t rows, std::size_t cols) {
    if (rows > MatrixLimits::kMaxRows || cols > MatrixLimits::kMaxCols)
        throw std::length_error("El resultado del operador excede los límites de la matriz");
}
ColumnSet concatenate(const ColumnSet& a, const ColumnSet& b) {
    ColumnSet result(a.width() + b.width());
    for (const auto col : a.indices()) result.set(col);
    for (const auto col : b.indices()) result.set(a.width() + col);
    return result;
}
}

BooleanMatrix phi(const BooleanMatrix& a, const BooleanMatrix& b) {
    nonempty(a); nonempty(b);
    if (a.rows() != b.rows())
        throw std::invalid_argument("Phi requiere el mismo número de filas");
    dimensions(a.rows(), a.cols() + b.cols());
    std::vector<ColumnSet> rows;
    rows.reserve(a.rows());
    for (std::size_t i = 0; i < a.rows(); ++i) rows.push_back(concatenate(a.row(i), b.row(i)));
    return {a.cols() + b.cols(), std::move(rows)};
}

BooleanMatrix theta(const BooleanMatrix& a, const BooleanMatrix& b) {
    nonempty(a); nonempty(b);
    // La división evita desbordar al multiplicar dimensiones.
    if (a.rows() > MatrixLimits::kMaxRows / b.rows())
        throw std::length_error("Theta excede el límite de filas");
    dimensions(a.rows() * b.rows(), a.cols() + b.cols());
    std::vector<ColumnSet> rows;
    rows.reserve(a.rows() * b.rows());
    for (const auto& ar : a.rowSets())
        for (const auto& br : b.rowSets()) rows.push_back(concatenate(ar, br));
    return {a.cols() + b.cols(), std::move(rows)};
}

BooleanMatrix gamma(const BooleanMatrix& a, const BooleanMatrix& b) {
    nonempty(a); nonempty(b);
    dimensions(a.rows() + b.rows(), a.cols() + b.cols());
    std::vector<ColumnSet> rows;
    rows.reserve(a.rows() + b.rows());
    const ColumnSet zerosA(a.cols()), zerosB(b.cols());
    for (const auto& row : a.rowSets()) rows.push_back(concatenate(row, zerosB));
    for (const auto& row : b.rowSets()) rows.push_back(concatenate(zerosA, row));
    return {a.cols() + b.cols(), std::move(rows)};
}

BooleanMatrix repeatPhi(const BooleanMatrix& matrix, std::size_t copies) {
    nonempty(matrix);
    if (copies == 0) throw std::invalid_argument("El número de copias debe ser positivo");
    if (copies > MatrixLimits::kMaxCols / matrix.cols())
        throw std::length_error("Phi iterado excede el límite de columnas");
    BooleanMatrix result = matrix;
    for (std::size_t i = 1; i < copies; ++i) result = phi(result, matrix);
    return result;
}

BooleanMatrix repeatGamma(const BooleanMatrix& matrix, std::size_t copies) {
    nonempty(matrix);
    if (copies == 0) throw std::invalid_argument("El número de copias debe ser positivo");
    if (copies > MatrixLimits::kMaxCols / matrix.cols() ||
        copies > MatrixLimits::kMaxRows / matrix.rows())
        throw std::length_error("Gamma iterado excede los límites de la matriz");
    BooleanMatrix result = matrix;
    for (std::size_t i = 1; i < copies; ++i) result = gamma(result, matrix);
    return result;
}

BooleanMatrix sortRowsAscending(const BooleanMatrix& matrix) {
    auto rows = matrix.rowSets();
    std::stable_sort(rows.begin(), rows.end(), [](const ColumnSet& a, const ColumnSet& b) {
        return a.count() < b.count();
    });
    return {matrix.cols(), std::move(rows)};
}
}  // namespace testores::domain
