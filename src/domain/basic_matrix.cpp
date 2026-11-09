#include "testores/domain/basic_matrix.hpp"

#include <algorithm>
#include <utility>

namespace testores::domain {

bool isSubrow(const ColumnSet& p, const ColumnSet& t) { return p.isSubsetOf(t) && p != t; }

BasicReduction reduceToBasic(const BooleanMatrix& matrix) {
    const std::size_t m = matrix.rows();
    BasicReduction result;
    result.decisions.assign(m, RowDecision{});

    // Paso 1: filas nulas y duplicadas (se conserva la primera aparición).
    std::vector<std::size_t> candidates;
    for (std::size_t i = 0; i < m; ++i) {
        const ColumnSet& r = matrix.row(i);
        if (r.none()) {
            result.decisions[i] = {RowFate::ZeroRow, i};
            continue;
        }
        bool duplicated = false;
        for (std::size_t c : candidates) {
            if (matrix.row(c) == r) {
                result.decisions[i] = {RowFate::Duplicate, c};
                duplicated = true;
                break;
            }
        }
        if (!duplicated) {
            candidates.push_back(i);
        }
    }

    // Paso 2: superfilas. Entre filas sin repetir la relación de subfila es un
    // orden parcial estricto (finito), así que sobreviven exactamente las filas
    // minimales y, por transitividad, toda superfila contiene a alguna de ellas.
    // Se usa una fila minimal como testigo para que la explicación cite una
    // fila que sí está en la matriz básica.
    std::vector<std::size_t> minimal;
    for (std::size_t i : candidates) {
        const bool hasSubrow = std::any_of(candidates.begin(), candidates.end(), [&](std::size_t j) {
            return i != j && isSubrow(matrix.row(j), matrix.row(i));
        });
        if (!hasSubrow) {
            minimal.push_back(i);
        }
    }
    std::vector<ColumnSet> kept;
    for (std::size_t i : candidates) {
        const auto witness = std::find_if(minimal.begin(), minimal.end(), [&](std::size_t j) {
            return i != j && isSubrow(matrix.row(j), matrix.row(i));
        });
        if (witness == minimal.end()) {
            kept.push_back(matrix.row(i));
        } else {
            result.decisions[i] = {RowFate::Superrow, *witness};
        }
    }

    result.basic = BooleanMatrix(matrix.cols(), std::move(kept));
    return result;
}

BooleanMatrix toBasicMatrix(const BooleanMatrix& matrix) { return reduceToBasic(matrix).basic; }

bool isBasicMatrix(const BooleanMatrix& matrix) {
    const std::size_t m = matrix.rows();
    for (std::size_t i = 0; i < m; ++i) {
        if (matrix.row(i).none()) {
            return false;
        }
        for (std::size_t j = 0; j < m; ++j) {
            if (i != j && matrix.row(j).isSubsetOf(matrix.row(i))) {
                return false;  // cubre subfila estricta y filas repetidas
            }
        }
    }
    return true;
}

}  // namespace testores::domain
