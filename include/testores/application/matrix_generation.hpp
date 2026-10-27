#pragma once

#include <cstddef>
#include <optional>

#include "testores/application/ports.hpp"
#include "testores/domain/basic_matrix.hpp"
#include "testores/domain/boolean_matrix.hpp"

namespace testores::application {

struct GenerationRequest {
    std::size_t rows = 0;
    std::size_t cols = 0;
    double onesProbability = 0.5;  // probabilidad de que cada entrada sea 1
};

struct GeneratedMatrices {
    domain::BooleanMatrix original;
    domain::BasicReduction reduction;
};

// Lanza std::invalid_argument si las dimensiones están fuera de
// [1, MatrixLimits] o la probabilidad fuera de [0, 1].
void validate(const GenerationRequest& request);

// Ejercicio 1: matriz booleana aleatoria m x n.
domain::BooleanMatrix generateRandomMatrix(const GenerationRequest& request, RandomSource& random);

// Ejercicio 1: matriz aleatoria + su matriz básica (con el detalle de la reducción).
GeneratedMatrices generateWithBasic(const GenerationRequest& request, RandomSource& random);

// Restricciones para buscar una matriz básica concreta (Ejercicio 2.1).
struct BasicMatrixConstraints {
    double minDensity = 0.0;
    std::size_t minRows = 1;
    std::size_t maxRows = domain::MatrixLimits::kMaxRows;
    bool requireEveryColumnUsed = true;  // ninguna columna nula en la básica
    std::size_t maxAttempts = 100000;
};

bool satisfies(const domain::BooleanMatrix& basic, const BasicMatrixConstraints& constraints);

struct SearchOutcome {
    std::optional<GeneratedMatrices> found;
    std::size_t attempts = 0;
};

// Genera matrices aleatorias hasta que su matriz básica cumpla las restricciones
// o se agoten los intentos. Determinista para una misma semilla.
SearchOutcome searchBasicMatrix(const GenerationRequest& request, const BasicMatrixConstraints& constraints,
                                RandomSource& random);

}  // namespace testores::application
