#include "matrix.hpp"
#include <iostream>
#include <cstdlib>
#include <ctime>

int main() {
    // Inicializar la semilla para la generación de números aleatorios
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    std::cout << "===================================================\n";
    std::cout << "   EJERCICIO 1: Generador y Reductor de Matrices   \n";
    std::cout << "===================================================\n\n";

    int m = 0, n = 0;

    // 1. Lectura de dimensiones desde teclado
    std::cout << "Ingrese el número de filas (m): ";
    std::cin >> m;
    std::cout << "Ingrese el número de columnas (n): ";
    std::cin >> n;

    // Validación básica de entrada
    if (m <= 0 || n <= 0) {
        std::cerr << "Error: Las dimensiones deben ser números enteros mayores a cero.\n";
        return 1;
    }

    // 2. Generación de la matriz aleatoria original
    Matrix originalMat;
    originalMat.generateRandom(m, n);

    std::cout << "\n--- MATRIZ ORIGINAL ALEATORIA (" << m << "x" << n << ") ---\n";
    originalMat.print();
    std::cout << "Densidad original: " << (originalMat.calculateDensity() * 100.0) << "%\n";

    // 3. Obtención de la Matriz Básica (eliminación de subfilas y superfilas)
    Matrix basicMat = originalMat.getBasicMatrix();

    std::cout << "\n--- MATRIZ BÁSICA RESULTANTE (" 
              << basicMat.getRows() << "x" << basicMat.getCols() << ") ---\n";
    
    if (basicMat.getRows() == 0) {
        std::cout << "(Todas las filas eran nulas o superfilas repetidas)\n";
    } else {
        basicMat.print();
    }

    // 4. Métrica de densidad y resumen de reducción
    double basicDensity = basicMat.calculateDensity();
    std::cout << "\n===================================================\n";
    std::cout << " RESUMEN DE RESULTADOS:\n";
    std::cout << " - Filas originales: " << m << "\n";
    std::cout << " - Filas básicas:    " << basicMat.getRows() << "\n";
    std::cout << " - Filas reducidas:  " << (m - basicMat.getRows()) << "\n";
    std::cout << " - Densidad Básica:  " << basicDensity << " (" << (basicDensity * 100.0) << "%)\n";
    std::cout << "===================================================\n";

    return 0;
}