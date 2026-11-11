#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "testores/domain/boolean_matrix.hpp"
#include "testores/domain/column_set.hpp"

namespace testores::domain {

// p es subfila de t  <=>  p <= t componente a componente y p != t.
// (Equivalentemente: el conjunto de unos de p está estrictamente contenido en el de t.)
// En particular, una fila nula es subfila de toda fila no nula.
bool isSubrow(const ColumnSet& p, const ColumnSet& t);

enum class RowFate {
    Kept,       // la fila pertenece a la matriz básica
    Duplicate,  // copia idéntica de una fila anterior
    Superrow    // existe otra fila que es subfila suya
};

struct RowDecision {
    RowFate fate = RowFate::Kept;
    // Índice (en la matriz original) de la fila que motivó la eliminación:
    // la primera aparición (Duplicate) o una subfila que se conserva en la
    // matriz básica (Superrow). Sin significado para Kept.
    std::size_t witness = 0;
};

struct BasicReduction {
    BooleanMatrix basic;
    std::vector<RowDecision> decisions;  // una por cada fila de la matriz original
};

// Calcula la matriz básica tal como la definen el apunte "Algo BT" y el
// artículo de YYC: las filas básicas (las que no tienen subfilas), sin
// repetición. Se eliminan las filas repetidas (se conserva la primera
// aparición) y las superfilas. El orden relativo de las filas conservadas se
// mantiene y el número de columnas no cambia.
//
// Si la matriz tiene una fila nula, esa fila es subfila de todas las filas no
// nulas, así que la matriz básica es una única fila nula. Así se conserva la
// propiedad fundamental de la reducción: la matriz y su básica tienen los
// mismos testores (en este caso, ninguno).
BasicReduction reduceToBasic(const BooleanMatrix& matrix);

BooleanMatrix toBasicMatrix(const BooleanMatrix& matrix);

// Una matriz es básica si ningún par de filas distintas cumple la relación de
// subfila ni son iguales. Una matriz con una fila nula sólo es básica si esa es
// su única fila.
bool isBasicMatrix(const BooleanMatrix& matrix);

// Índice de la primera fila nula, si existe. Con una fila nula ningún
// conjunto de rasgos es testor.
std::optional<std::size_t> firstNullRow(const BooleanMatrix& matrix);

}  // namespace testores::domain
