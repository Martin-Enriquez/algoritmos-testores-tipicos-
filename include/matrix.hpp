#pragma once

#include <vector>
#include <iostream>

class Matrix {
private:
    int rows;
    int cols;
    std::vector<std::vector<int>> data;

    // Métodos auxiliares privados para la reducción
    bool isNullRow(const std::vector<int>& row) const;
    bool isSubrow(const std::vector<int>& p, const std::vector<int>& t) const;

public:
    // Constructores
    Matrix();
    Matrix(int m, int n);
    Matrix(const std::vector<std::vector<int>>& grid);

    // --- MÉTODOS DEL EJERCICIO 1 ---

    // 1. Generar matriz booleana aleatoria de m x n
    void generateRandom(int m, int n);

    // 2. Reducir a Matriz Básica (elimina filas nulas, repetidas y superfilas)
    Matrix getBasicMatrix() const;

    // 3. Calcular la densidad de la matriz (unos / (m * n))
    double calculateDensity() const;

    // Métodos de utilidad / visualización
    void print() const;
    int getRows() const { return rows; }
    int getCols() const { return cols; }
    const std::vector<std::vector<int>>& getData() const { return data; }
};