#include "testores/application/experiments.hpp"

#include <algorithm>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
#include "median_duration.hpp"
#include "testores/domain/basic_matrix.hpp"
#include "testores/domain/bt.hpp"
#include "testores/domain/matrix_operators.hpp"
#include "testores/domain/yyc.hpp"

namespace testores::application {
namespace {
std::size_t checkedMultiply(std::size_t a, std::size_t b) {
    if (b != 0 && a > std::numeric_limits<std::size_t>::max() / b)
        throw std::overflow_error("La cardinalidad teórica no cabe en size_t");
    return a * b;
}
std::size_t checkedAdd(std::size_t a, std::size_t b) {
    if (a > std::numeric_limits<std::size_t>::max() - b)
        throw std::overflow_error("La cardinalidad teórica no cabe en size_t");
    return a + b;
}
std::size_t power(std::size_t base, std::size_t exponent) {
    std::size_t result = 1;
    for (std::size_t i = 0; i < exponent; ++i) result = checkedMultiply(result, base);
    return result;
}
}

domain::BooleanMatrix buildDualMatrix(const domain::BooleanMatrix& a) {
    if (a.empty() || !domain::isBasicMatrix(a) || domain::firstNullRow(a))
        throw std::invalid_argument("La dualidad requiere una matriz básica no vacía sin fila nula");
    return {a.cols(), domain::YycSolver::solve(a)};
}

std::vector<ExperimentCase> buildExperimentCases(const domain::BooleanMatrix& a) {
    const auto b = buildDualMatrix(a);
    const auto base = domain::theta(a, b);
    // TT(theta(A,B)) = TT(A) en bloque izquierdo U TT(B) en derecho.
    // Por dualidad TT(B) son las filas de A, al ser A básica.
    std::vector<std::size_t> sizes;
    for (const auto& t : b.rowSets()) sizes.push_back(t.count());
    for (const auto& row : a.rowSets()) sizes.push_back(row.count());
    std::vector<ExperimentCase> cases;
    for (std::size_t n = 1; n <= 5; ++n) {
        std::size_t count = 0;
        for (const auto size : sizes) count = checkedAdd(count, power(n, size));
        cases.push_back({"phi", n, domain::repeatPhi(base,n), count});
    }
    for (std::size_t n = 1; n <= 4; ++n)
        cases.push_back({"gamma", n, domain::repeatGamma(base,n), power(sizes.size(),n)});
    return cases;
}

ExperimentMeasurements measureExperiment(const ExperimentCase& experiment, Clock& clock,
                                         const ExperimentOptions& options, ExperimentProgress* progress) {
    if (options.repetitions == 0 || options.repetitions > 10000 || options.timeLimit.count() < 0)
        throw std::invalid_argument("Repeticiones fuera de [1,10000] o límite negativo");
    ExperimentMeasurements measurements;
    const auto ascending = domain::sortRowsAscending(experiment.matrix);
    std::optional<std::vector<domain::ColumnSet>> reference;
    for (std::size_t method = 0; method < measurements.size(); ++method) {
        auto& measurement = measurements[method];
        const auto& matrix = method % 2 == 0 ? experiment.matrix : ascending;
        std::vector<std::chrono::nanoseconds> samples;
        samples.reserve(options.repetitions);
        bool expired = false;
        auto execute = [&]() {
            const auto start = clock.now();
            const auto stop = [&]() {
                expired = options.timeLimit.count() > 0 && clock.now() - start >= options.timeLimit;
                return expired;
            };
            std::vector<domain::ColumnSet> typical;
            if (method < 2) {
                domain::YycOptions solverOptions;
                if (options.timeLimit.count() > 0) solverOptions.shouldStop = stop;
                typical = domain::YycSolver::solve(matrix,solverOptions);
            } else {
                domain::BtOptions solverOptions;
                if (options.timeLimit.count() > 0) solverOptions.shouldStop = stop;
                // Se conserva el mismo límite por defecto que en la CLI.
                auto result = domain::BtSolver::solve(matrix,solverOptions);
                measurement.candidates = result.stats.candidatesEvaluated;
                typical = std::move(result.typicalTestors);
            }
            const auto elapsed = clock.now() - start;
            // Se comprueba también el tiempo al terminar: la consulta de
            // cancelación periódica no necesariamente coincide con el final.
            if (options.timeLimit.count() > 0 && elapsed >= options.timeLimit) {
                expired = true;
                throw std::runtime_error("Se alcanzó el límite de tiempo");
            }
            if (typical.size() != experiment.expectedTypical)
                throw std::logic_error("La cardinalidad calculada contradice la fórmula del operador");
            if (reference && *reference != typical)
                throw std::logic_error("Los conjuntos difieren entre órdenes, métodos o repeticiones");
            if (!reference) reference = std::move(typical);
            return elapsed;
        };
        try {
            execute();  // calentamiento no incluido en la mediana
            for (std::size_t rep = 0; rep < options.repetitions; ++rep) {
                samples.push_back(execute());
                ++measurement.completedRuns;
            }
            measurement.median = detail::medianDuration(samples);
            measurement.status = "ok";
        } catch (const domain::BtResourceLimitExceeded& error) {
            measurement.status = expired ? "limite" : "recurso";
            measurement.detail = error.what();
        } catch (const domain::YycResourceLimitExceeded& error) {
            measurement.status = expired ? "limite" : "recurso";
            measurement.detail = error.what();
        } catch (const std::runtime_error& error) {
            if (!expired) throw;
            measurement.status = "limite";
            measurement.detail = error.what();
        }
        if (progress) progress->onMeasurement(experiment,method,measurement);
    }
    return measurements;
}
}  // namespace testores::application
