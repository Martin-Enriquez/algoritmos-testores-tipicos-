#include "testores/application/bt_runner.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace testores::application {

using domain::BtSolver;
using domain::ColumnSet;

namespace {

void checkTypicality(BtRunResult& result) {
    if (result.run.testorsTruncated) {
        // Sin la lista completa de testores el filtro a posteriori no es concluyente.
        result.typicalByMinimality.clear();
        result.typicalityProceduresAgree = false;
        return;
    }
    std::vector<ColumnSet> found;
    found.reserve(result.run.testors.size());
    for (const auto& testor : result.run.testors) {
        found.push_back(testor.set);
    }
    result.typicalByMinimality = domain::minimalSets(found);
    result.typicalityProceduresAgree = result.typicalByMinimality == result.run.typicalTestors;
}

}  // namespace

BtRunResult runBt(const domain::BooleanMatrix& matrix, Clock& clock, const domain::BtOptions& options) {
    BtRunResult result;
    const std::chrono::nanoseconds start = clock.now();
    result.run = BtSolver::solve(matrix, options);
    result.total = clock.now() - start;
    checkTypicality(result);
    return result;
}

BtRunResult benchmarkBt(const domain::BooleanMatrix& matrix, Clock& clock, std::size_t repetitions,
                        const domain::BtOptions& options) {
    if (repetitions == 0) {
        throw std::invalid_argument("El número de repeticiones debe ser al menos 1");
    }
    domain::BtOptions quiet = options;
    quiet.recordTrace = false;

    BtRunResult reference = runBt(matrix, clock, quiet);
    std::vector<std::chrono::nanoseconds> samples{reference.total};
    samples.reserve(repetitions);
    for (std::size_t rep = 1; rep < repetitions; ++rep) {
        const std::chrono::nanoseconds start = clock.now();
        (void)BtSolver::solve(matrix, quiet);
        samples.push_back(clock.now() - start);
    }
    auto middle = samples.begin() + static_cast<std::ptrdiff_t>(samples.size() / 2);
    std::nth_element(samples.begin(), middle, samples.end());
    reference.total = *middle;
    reference.repetitions = repetitions;
    if (options.recordTrace) {
        // Conservar la traza pedida (se obtiene en una corrida aparte, no cronometrada).
        domain::BtResult traced = BtSolver::solve(matrix, options);
        reference.run.trace = std::move(traced.trace);
        reference.run.traceTruncated = traced.traceTruncated;
    }
    return reference;
}

}  // namespace testores::application
