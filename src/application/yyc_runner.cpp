#include "testores/application/yyc_runner.hpp"

#include <algorithm>
#include <stdexcept>

#include "median_duration.hpp"

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
    // Cada fila guarda una muestra por repetición: limitar sólo R no impide
    // que una matriz alta reserve varios gigabytes para los tiempos.
    constexpr std::size_t maxTimingSamples = 10000000;
    if (matrix.rows() > 0 && repetitions > maxTimingSamples / matrix.rows()) {
        throw std::invalid_argument("La medición de YYC supera 10 000 000 muestras; reduzca las repeticiones");
    }
    YycRunOptions quiet = options;
    quiet.solver.recordTrace = false;

    // samples[i] = tiempo acumulado tras la fila i en cada repetición.
    YycRunResult reference = runYyc(matrix, clock, nullptr, quiet);
    quiet.maxSnapshotSets = 0;  // las repeticiones sólo aportan tiempos
    std::vector<std::vector<std::chrono::nanoseconds>> samples(reference.rows.size());
    for (auto& row : samples) {
        row.reserve(repetitions);
    }
    auto collect = [&samples](const YycRunResult& run) {
        for (std::size_t i = 0; i < run.rows.size(); ++i) {
            samples[i].push_back(run.rows[i].cumulative);
        }
    };
    collect(reference);
    for (std::size_t rep = 1; rep < repetitions; ++rep) {
        collect(runYyc(matrix, clock, nullptr, quiet));
    }

    // Mediana del tiempo acumulado de cada fila (no la suma de medianas por
    // fila: la mediana no es aditiva). La columna sigue siendo creciente: en
    // cada repetición el acumulado de la fila i+1 es >= al de la fila i, y un
    // mismo estadístico de orden conserva esa desigualdad.
    for (std::size_t i = 0; i < samples.size(); ++i) {
        reference.rows[i].cumulative = detail::medianDuration(samples[i]);
    }
    reference.total = reference.rows.empty() ? std::chrono::nanoseconds{0} : reference.rows.back().cumulative;
    reference.repetitions = repetitions;
    return reference;
}

}  // namespace testores::application
