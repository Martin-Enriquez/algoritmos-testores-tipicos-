#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <string>
#include <vector>
#include "testores/application/ports.hpp"
#include "testores/domain/boolean_matrix.hpp"

namespace testores::application {

struct ExperimentCase {
    std::string family;
    std::size_t copies;
    domain::BooleanMatrix matrix;
    std::size_t expectedTypical;
};

struct ExperimentOptions {
    std::size_t repetitions = 9;
    // Por ejecución, no por tabla; cero desactiva el límite.
    std::chrono::nanoseconds timeLimit = std::chrono::seconds(10);
};

struct ExperimentMeasurement {
    std::string status = "pendiente";
    std::chrono::nanoseconds median{0};
    std::size_t completedRuns = 0;
    std::size_t candidates = 0;  // sólo BT
    std::string detail;
};

// Orden fijo: YYC original, YYC ascendente, BT original, BT ascendente.
using ExperimentMeasurements = std::array<ExperimentMeasurement, 4>;

class ExperimentProgress {
public:
    virtual ~ExperimentProgress() = default;
    virtual void onMeasurement(const ExperimentCase& experiment, std::size_t index,
                               const ExperimentMeasurement& measurement) = 0;
};

// Construye B y las nueve matrices de las tablas 5/6 usando A y sus típicos.
// La entrada debe ser básica, sin fila nula. Orden de bloques/fila estable.
domain::BooleanMatrix buildDualMatrix(const domain::BooleanMatrix& a);
std::vector<ExperimentCase> buildExperimentCases(const domain::BooleanMatrix& a);

// Una ejecución de calentamiento y N mediciones de solve completo, sin traza.
// Verifica cardinalidad teórica y conjuntos entre repeticiones/órdenes/métodos.
// Un resultado interrumpido se marca 'limite' y no se presenta como completo.
ExperimentMeasurements measureExperiment(const ExperimentCase& experiment, Clock& clock,
                                         const ExperimentOptions& options,
                                         ExperimentProgress* progress = nullptr);

}  // namespace testores::application
