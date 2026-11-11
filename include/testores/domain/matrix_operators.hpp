#pragma once

#include "testores/domain/boolean_matrix.hpp"

namespace testores::domain {

// Concatena cada fila de a con cada fila de b (a como bloque izquierdo).
// Orden: a1|b1, a1|b2, ..., a2|b1, ... . No reduce ni reordena.
// Dimensiones: (a.rows() * b.rows()) x (a.cols() + b.cols()).
// Si ambas entradas son básicas, el resultado también es básico.
// Rechaza matrices vacías y resultados que excedan MatrixLimits.
BooleanMatrix theta(const BooleanMatrix& a, const BooleanMatrix& b);

// Concatenación horizontal fila por fila; requiere el mismo número de filas.
BooleanMatrix phi(const BooleanMatrix& a, const BooleanMatrix& b);

// phi^copies(a): copies bloques horizontales idénticos; phi^1(a) = a.
// Conserva las filas y su orden. Rechaza cero copias y exceso de columnas.
BooleanMatrix phiPower(const BooleanMatrix& a, std::size_t copies);

// Bloques diagonales [a 0; 0 b]; el resto de entradas son ceros.
BooleanMatrix gamma(const BooleanMatrix& a, const BooleanMatrix& b);

// gamma^copies(a): copies bloques diagonales; gamma^1(a) = a.
// Dimensiones: (copies * a.rows()) x (copies * a.cols()).
BooleanMatrix gammaPower(const BooleanMatrix& a, std::size_t copies);

// Nombres usados por el ejecutable de experimentos de main.
BooleanMatrix repeatPhi(const BooleanMatrix& matrix, std::size_t copies);
BooleanMatrix repeatGamma(const BooleanMatrix& matrix, std::size_t copies);
BooleanMatrix sortRowsAscending(const BooleanMatrix& matrix);

// Orden ascendente por cantidad de unos. En empates conserva el orden original.
// No cambia las columnas ni elimina filas.
BooleanMatrix sortRowsByOnes(const BooleanMatrix& matrix);

}  // namespace testores::domain
