#include "testores/application/matrix_generation.hpp"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace testores::application {

using domain::BooleanMatrix;
using domain::ColumnSet;
using domain::MatrixLimits;

void validate(const GenerationRequest& request) {
    if (request.rows < 1 || request.rows > MatrixLimits::kMaxRows) {
        throw std::invalid_argument("El número de filas debe estar entre 1 y " +
                                    std::to_string(MatrixLimits::kMaxRows));
    }
    if (request.cols < 1 || request.cols > MatrixLimits::kMaxCols) {
        throw std::invalid_argument("El número de columnas debe estar entre 1 y " +
                                    std::to_string(MatrixLimits::kMaxCols));
    }
    // La comparación negada también rechaza NaN.
    if (!(request.onesProbability >= 0.0 && request.onesProbability <= 1.0)) {
        throw std::invalid_argument("La probabilidad de unos debe estar en [0, 1]");
    }
}

BooleanMatrix generateRandomMatrix(const GenerationRequest& request, RandomSource& random) {
    validate(request);
    std::vector<ColumnSet> rows;
    rows.reserve(request.rows);
    for (std::size_t i = 0; i < request.rows; ++i) {
        ColumnSet row(request.cols);
        for (std::size_t j = 0; j < request.cols; ++j) {
            row.set(j, random.bernoulli(request.onesProbability));
        }
        rows.push_back(std::move(row));
    }
    return BooleanMatrix(request.cols, std::move(rows));
}

GeneratedMatrices generateWithBasic(const GenerationRequest& request, RandomSource& random) {
    BooleanMatrix original = generateRandomMatrix(request, random);
    domain::BasicReduction reduction = domain::reduceToBasic(original);
    return {std::move(original), std::move(reduction)};
}

bool satisfies(const BooleanMatrix& basic, const BasicMatrixConstraints& constraints) {
    if (basic.rows() < constraints.minRows || basic.rows() > constraints.maxRows) {
        return false;
    }
    if (basic.density() < constraints.minDensity) {
        return false;
    }
    if (constraints.requireEveryColumnUsed) {
        ColumnSet used(basic.cols());
        for (const ColumnSet& r : basic.rowSets()) {
            used |= r;
        }
        if (used.count() != basic.cols()) {
            return false;
        }
    }
    return true;
}

SearchOutcome searchBasicMatrix(const GenerationRequest& request, const BasicMatrixConstraints& constraints,
                                RandomSource& random) {
    validate(request);
    if (constraints.minRows > constraints.maxRows) {
        throw std::invalid_argument("minRows no puede ser mayor que maxRows");
    }
    SearchOutcome outcome;
    while (outcome.attempts < constraints.maxAttempts) {
        ++outcome.attempts;
        GeneratedMatrices candidate = generateWithBasic(request, random);
        if (satisfies(candidate.reduction.basic, constraints)) {
            outcome.found = std::move(candidate);
            break;
        }
    }
    return outcome;
}

}  // namespace testores::application
