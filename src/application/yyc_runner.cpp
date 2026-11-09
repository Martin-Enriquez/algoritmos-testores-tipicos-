#include "testores/application/yyc_runner.hpp"

#include <algorithm>
#include <stdexcept>

namespace testores::application {

using domain::ColumnSet;
using domain::YycSolver;

namespace {

std::vector<ColumnSet> canonical(std::vector<ColumnSet> sets) {
    std::sort(sets.begin(), sets.end(), domain::canonicalLess);
    return sets;
}

}  // namespace

YycRunResult runYyc(const domain::BooleanMatrix& matrix, Clock& clock, YycProgressListener* listener,
                    const YycRunOptions& options) {
    YycSolver solver(matrix, options.solver);
    YycRunResult result;
    result.rows.reserve(matrix.rows());

    std::chrono::nanoseconds cumulative{0};
    std::size_t storedSets = 0;
    while (!solver.finished()) {
        const std::chrono::nanoseconds start = clock.now();
        const domain::YycStepStats stats = solver.processNextRow();
        cumulative += clock.now() - start;

        YycRowRecord record;
        record.rowIndex = stats.rowIndex;
        record.testorCount = solver.current().size();
        record.cumulative = cumulative;
        if (storedSets + record.testorCount <= options.maxSnapshotSets) {
            record.testors = canonical(solver.current());
            storedSets += record.testorCount;
        } else {
            record.testorsOmitted = true;
        }
        result.rows.push_back(std::move(record));

        if (listener != nullptr) {
            listener->onRowProcessed(
                {stats.rowIndex, matrix.rows(), solver.current(), cumulative, stats, solver.lastTrace()});
        }
    }
    result.typicalTestors = canonical(solver.current());
    result.total = cumulative;
    return result;
}

YycRunResult benchmarkYyc(const domain::BooleanMatrix& matrix, Clock& clock, std::size_t repetitions,
                          const YycRunOptions& options) {
    if (repetitions == 0) {
        throw std::invalid_argument("El número de repeticiones debe ser al menos 1");
    }
    YycRunOptions quiet = options;
    quiet.solver.recordTrace = false;

    // samples[i] = duraciones de la fila i (no acumuladas) en cada repetición.
    YycRunResult reference = runYyc(matrix, clock, nullptr, quiet);
    quiet.maxSnapshotSets = 0;  // las repeticiones sólo aportan tiempos
    std::vector<std::vector<std::chrono::nanoseconds>> samples(reference.rows.size());
    auto collect = [&samples](const YycRunResult& run) {
        std::chrono::nanoseconds previous{0};
        for (std::size_t i = 0; i < run.rows.size(); ++i) {
            samples[i].push_back(run.rows[i].cumulative - previous);
            previous = run.rows[i].cumulative;
        }
    };
    collect(reference);
    for (std::size_t rep = 1; rep < repetitions; ++rep) {
        collect(runYyc(matrix, clock, nullptr, quiet));
    }

    // Se acumulan las medianas por fila para que la columna acumulada sea monótona.
    std::chrono::nanoseconds cumulative{0};
    for (std::size_t i = 0; i < samples.size(); ++i) {
        auto middle = samples[i].begin() + static_cast<std::ptrdiff_t>(samples[i].size() / 2);
        std::nth_element(samples[i].begin(), middle, samples[i].end());
        cumulative += *middle;
        reference.rows[i].cumulative = cumulative;
    }
    reference.total = cumulative;
    reference.repetitions = repetitions;
    return reference;
}

}  // namespace testores::application
