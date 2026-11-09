#pragma once

#include <cstddef>
#include <optional>

#include "testores/domain/boolean_matrix.hpp"
#include "testores/domain/column_set.hpp"

namespace testores::domain {

// T es testor de M  <=>  en la submatriz M|T no hay filas nulas, es decir,
// toda fila de M tiene al menos un 1 en alguna columna de T.
bool isTestor(const BooleanMatrix& matrix, const ColumnSet& features);

// Fila típica de la columna j respecto de T: fila con 1 en j y 0 en el resto de T.
// T es compatible en las primeras `rowCount` filas si cada columna de T tiene
// una fila típica entre ellas (la submatriz contiene, salvo permutación de
// filas, a la matriz identidad de orden |T|).
bool isCompatibleSet(const BooleanMatrix& matrix, const ColumnSet& features, std::size_t rowCount);

// Primera columna de T (si existe) que no tiene fila típica en las primeras
// `rowCount` filas. Sirve para explicar por qué un conjunto no es compatible.
std::optional<std::size_t> columnWithoutTypicalRow(const BooleanMatrix& matrix, const ColumnSet& features,
                                                   std::size_t rowCount);

// T es testor típico  <=>  T es testor y ningún subconjunto propio lo es
// <=>  T es testor y es compatible (cada columna tiene fila típica).
bool isTypicalTestor(const BooleanMatrix& matrix, const ColumnSet& features);

}  // namespace testores::domain
