# Algoritmos para el Cálculo de Testores Típicos en Matrices Básicas

Proyecto académico de **Matemáticas Discretas** enfocado en la implementación, evaluación y comparación del comportamiento computacional de algoritmos para la identificación de Testores Típicos (TTA) en matrices básicas.

---

## 📌 Descripción General

El objetivo principal es implementar y evaluar computacionalmente los algoritmos **YYC** (*Yablonsky & Compatible sets*) y **BT** (*Backtracking*) para el cálculo de testores típicos. Además, el proyecto incluye:

- Generación de matrices sintéticas y cálculo de densidad.
- Implementación de operadores de construcción de matrices ($\phi$, $\theta$, $\gamma$).
- Análisis comparativo de rendimiento variando la densidad y el ordenamiento de filas (orden original vs. orden ascendente por cantidad de 1s).
- *(Opcional / Grupo de 4)* Algoritmo de búsqueda exhaustiva (fuerza bruta) para validación de resultados.

---

## 🛠️ Requisitos e Instalación

### Requisitos previos
- Compilador de C++ con soporte para **C++17** o superior (`g++`, `clang++`, o MSVC).
- **CMake** (versión 3.10 o superior) o `make`.

### Compilador rápido (g++)
```bash
g++ -std=c++17 -Iinclude src/*.cpp -o ejecutable
./ejecutable