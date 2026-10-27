#pragma once

#include <cstddef>
#include <vector>

#include "testores/domain/boolean_matrix.hpp"
#include "testores/domain/column_set.hpp"

namespace testores::domain {

// p es subfila de t  <=>  p <= t componente a componente y p != t.
// (Equivalentemente: el conjunto de unos de p está estrictamente contenido en el de t.)
bool isSubrow(const ColumnSet& p, const ColumnSet& t);

enum class RowFate {
    Kept,       // la fila pertenece a la matriz básica
    ZeroRow,    // fila nula: no puede ser cubierta por ningún conjunto de rasgos
    Duplicate,  // copia idéntica de una fila anterior
    Superrow    // existe otra fila que es subfila suya
};

struct RowDecision {
    RowFate fate = RowFate::Kept;
    // Índice (en la matriz original) de la fila que motivó la eliminación:
    // la primera aparición (Duplicate) o una subfila que se conserva en la
    // matriz básica (Superrow). Sin significado para Kept y ZeroRow.
    std::size_t witness = 0;
};

struct BasicReduction {
    BooleanMatrix basic;
    std::vector<RowDecision> decisions;  // una por cada fila de la matriz original
};

// Calcula la matriz básica: elimina filas nulas, filas repetidas (se conserva
// la primera aparición) y superfilas. El orden relativo de las filas
// conservadas se mantiene. El número de columnas no cambia.
BasicReduction reduceToBasic(const BooleanMatrix& matrix);

BooleanMatrix toBasicMatrix(const BooleanMatrix& matrix);

// Una matriz es básica si no tiene filas nulas y ningún par de filas distintas
// cumple la relación de subfila ni son iguales.
bool isBasicMatrix(const BooleanMatrix& matrix);

}  // namespace testores::domain
