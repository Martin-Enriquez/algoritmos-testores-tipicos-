#include "matrix.hpp"
#include <cstdlib>
#include <ctime>
#include <algorithm>

// Constructores
Matrix::Matrix() : rows(0), cols(0) {}

Matrix::Matrix(int m, int n) : rows(m), cols(n), data(m, std::vector<int>(n, 0)) {}

Matrix::Matrix(const std::vector<std::vector<int>>& grid) {
    data = grid;
    rows = static_cast<int>(grid.size());
    cols = rows > 0 ? static_cast<int>(grid[0].size()) : 0;
}

// 1. Generar matriz booleana aleatoria de m x n (valores 0 o 1)
void Matrix::generateRandom(int m, int n) {
    rows = m;
    cols = n;
    data.assign(m, std::vector<int>(n, 0));
    
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            data[i][j] = std::rand() % 2;
        }
    }
}

// Método auxiliar: verifica si una fila es completamente nula (todos 0s)
bool Matrix::isNullRow(const std::vector<int>& row) const {
    for (int val : row) {
        if (val != 0) return false;
    }
    return true;
}

// Método auxiliar: verifica si la fila 'p' es SUBFILA de la fila 't'
// Condición 1: Todo 1 en p también es 1 en t (p_j = 1 => t_j = 1)
// Condición 2: Existe al menos un 1 en t donde p tiene 0
bool Matrix::isSubrow(const std::vector<int>& p, const std::vector<int>& t) const {
    bool has_extra_one = false;
    for (size_t j = 0; j < p.size(); ++j) {
        if (p[j] == 1 && t[j] == 0) {
            return false; // p tiene un 1 que t no posee -> no es subfila
        }
        if (t[j] == 1 && p[j] == 0) {
            has_extra_one = true;
        }
    }
    return has_extra_one;
}

// 2. Reducir a Matriz Básica (elimina filas nulas, repetidas y superfilas)
Matrix Matrix::getBasicMatrix() const {
    if (rows == 0 || cols == 0) return Matrix();

    // Paso A: Filtrar filas nulas y duplicadas
    std::vector<std::vector<int>> unique_rows;
    for (int i = 0; i < rows; ++i) {
        if (isNullRow(data[i])) continue; // omitir filas de puros ceros
        
        bool duplicate = false;
        for (const auto& u : unique_rows) {
            if (u == data[i]) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) {
            unique_rows.push_back(data[i]);
        }
    }

    // Paso B: Filtrar superfilas (si p es subfila de t, t es superfila y se descarta)
    std::vector<std::vector<int>> basic_rows;
    for (size_t i = 0; i < unique_rows.size(); ++i) {
        bool is_basic = true;
        for (size_t j = 0; j < unique_rows.size(); ++j) {
            if (i == j) continue;
            // Si otra fila 'j' es SUBFILA de la fila 'i', entonces 'i' es superfila y no es básica
            if (isSubrow(unique_rows[j], unique_rows[i])) {
                is_basic = false;
                break;
            }
        }
        if (is_basic) {
            basic_rows.push_back(unique_rows[i]);
        }
    }

    return Matrix(basic_rows);
}

// 3. Calcular densidad de la matriz: (total de 1s) / (m * n)
double Matrix::calculateDensity() const {
    if (rows == 0 || cols == 0) return 0.0;
    
    int ones_count = 0;
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            if (data[i][j] == 1) {
                ones_count++;
            }
        }
    }
    return static_cast<double>(ones_count) / (rows * cols);
}

// Método de utilidad para imprimir la matriz
void Matrix::print() const {
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            std::cout << data[i][j] << " ";
        }
        std::cout << "\n";
    }
}
