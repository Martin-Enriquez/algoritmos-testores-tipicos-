#pragma once

#include <cstddef>
#include "testores/domain/boolean_matrix.hpp"

namespace testores::domain {

// Operadores de Alba-Cabrera et al. (2016). No reducen ni reordenan filas.
// Todos validan las dimensiones antes de reservar memoria.
BooleanMatrix phi(const BooleanMatrix& a, const BooleanMatrix& b);
BooleanMatrix theta(const BooleanMatrix& a, const BooleanMatrix& b);
BooleanMatrix gamma(const BooleanMatrix& a, const BooleanMatrix& b);
BooleanMatrix repeatPhi(const BooleanMatrix& matrix, std::size_t copies);
BooleanMatrix repeatGamma(const BooleanMatrix& matrix, std::size_t copies);
BooleanMatrix sortRowsAscending(const BooleanMatrix& matrix);

}  // namespace testores::domain
