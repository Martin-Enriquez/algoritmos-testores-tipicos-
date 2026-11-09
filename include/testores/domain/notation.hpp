#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "testores/domain/column_set.hpp"

// Notación del curso: las columnas son los rasgos x1, ..., xn (base 1).
namespace testores::domain {

std::string featureName(std::size_t col);

// {x1, x3, x5}; el conjunto vacío se escribe {}.
std::string formatFeatureSet(const ColumnSet& set);

// {x1, x3}, {x2, x4}; "∅" si la lista está vacía.
std::string formatFeatureSets(const std::vector<ColumnSet>& sets);

// Notación binaria estándar: "10101" (posición j = 1 si x_{j+1} pertenece).
std::string toBinaryString(const ColumnSet& set);

}  // namespace testores::domain
