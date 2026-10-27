#pragma once

#include <cstddef>
#include <vector>

#include "testores/domain/column_set.hpp"

namespace testores::domain {

// Límites duros del sistema. Protegen contra entradas que agotarían memoria o
// tiempo de cómputo (archivos manipulados, parámetros absurdos, etc.).
struct MatrixLimits {
    static constexpr std::size_t kMaxRows = 100000;
    static constexpr std::size_t kMaxCols = ColumnSet::kMaxWidth;
};

// Matriz booleana inmutable m x n. Cada fila se almacena como el conjunto de
// columnas en las que tiene un 1.
class BooleanMatrix {
public:
    BooleanMatrix() = default;

    // Lanza std::invalid_argument si alguna fila no tiene ancho `cols` o si se
    // exceden los límites de MatrixLimits.
    BooleanMatrix(std::size_t cols, std::vector<ColumnSet> rows);

    // Construye a partir de una cuadrícula de 0/1. Lanza std::invalid_argument
    // si hay valores distintos de 0/1, filas de distinto largo o límites excedidos.
    static BooleanMatrix fromGrid(const std::vector<std::vector<int>>& grid);

    std::size_t rows() const noexcept { return rows_.size(); }
    std::size_t cols() const noexcept { return cols_; }
    bool empty() const noexcept { return rows_.empty() || cols_ == 0; }

    const ColumnSet& row(std::size_t i) const;
    const std::vector<ColumnSet>& rowSets() const noexcept { return rows_; }
    bool at(std::size_t i, std::size_t j) const { return row(i).test(j); }

    std::size_t onesCount() const noexcept;

    // Densidad = (número de unos) / (m * n). Es 0 para la matriz vacía.
    double density() const noexcept;

    std::vector<std::vector<int>> toGrid() const;

    friend bool operator==(const BooleanMatrix& a, const BooleanMatrix& b) noexcept {
        return a.cols_ == b.cols_ && a.rows_ == b.rows_;
    }

private:
    std::size_t cols_ = 0;
    std::vector<ColumnSet> rows_;
};

}  // namespace testores::domain
