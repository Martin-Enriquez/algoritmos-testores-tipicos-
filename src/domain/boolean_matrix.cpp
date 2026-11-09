#include "testores/domain/boolean_matrix.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace testores::domain {

BooleanMatrix::BooleanMatrix(std::size_t cols, std::vector<ColumnSet> rows)
    : cols_(cols), rows_(std::move(rows)) {
    if (cols_ > MatrixLimits::kMaxCols) {
        throw std::invalid_argument("La matriz excede el máximo de " +
                                    std::to_string(MatrixLimits::kMaxCols) + " columnas");
    }
    if (rows_.size() > MatrixLimits::kMaxRows) {
        throw std::invalid_argument("La matriz excede el máximo de " +
                                    std::to_string(MatrixLimits::kMaxRows) + " filas");
    }
    for (const ColumnSet& r : rows_) {
        if (r.width() != cols_) {
            throw std::invalid_argument("Todas las filas deben tener el mismo número de columnas");
        }
    }
}

BooleanMatrix BooleanMatrix::fromGrid(const std::vector<std::vector<int>>& grid) {
    if (grid.empty()) {
        return BooleanMatrix();
    }
    const std::size_t cols = grid.front().size();
    if (cols > MatrixLimits::kMaxCols) {
        throw std::invalid_argument("La matriz excede el máximo de columnas permitido");
    }
    std::vector<ColumnSet> rows;
    rows.reserve(grid.size());
    for (const auto& line : grid) {
        if (line.size() != cols) {
            throw std::invalid_argument("Todas las filas deben tener el mismo número de columnas");
        }
        ColumnSet r(cols);
        for (std::size_t j = 0; j < cols; ++j) {
            if (line[j] != 0 && line[j] != 1) {
                throw std::invalid_argument("Una matriz booleana sólo puede contener 0 y 1");
            }
            r.set(j, line[j] == 1);
        }
        rows.push_back(std::move(r));
    }
    return BooleanMatrix(cols, std::move(rows));
}

const ColumnSet& BooleanMatrix::row(std::size_t i) const {
    if (i >= rows_.size()) {
        throw std::out_of_range("BooleanMatrix::row: fila fuera de rango");
    }
    return rows_[i];
}

std::size_t BooleanMatrix::onesCount() const noexcept {
    std::size_t total = 0;
    for (const ColumnSet& r : rows_) {
        total += r.count();
    }
    return total;
}

double BooleanMatrix::density() const noexcept {
    if (empty()) {
        return 0.0;
    }
    return static_cast<double>(onesCount()) / (static_cast<double>(rows_.size()) * static_cast<double>(cols_));
}

std::vector<std::vector<int>> BooleanMatrix::toGrid() const {
    std::vector<std::vector<int>> grid(rows_.size(), std::vector<int>(cols_, 0));
    for (std::size_t i = 0; i < rows_.size(); ++i) {
        for (std::size_t j = 0; j < cols_; ++j) {
            grid[i][j] = rows_[i].test(j) ? 1 : 0;
        }
    }
    return grid;
}

}  // namespace testores::domain
