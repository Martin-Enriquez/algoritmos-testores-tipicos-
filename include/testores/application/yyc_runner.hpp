#pragma once

#include <chrono>
#include <cstddef>
#include <vector>

#include "testores/application/ports.hpp"
#include "testores/domain/boolean_matrix.hpp"
#include "testores/domain/column_set.hpp"
#include "testores/domain/yyc.hpp"

namespace testores::application {

struct YycRowRecord {
    std::size_t rowIndex = 0;                // base 0
    std::size_t testorCount = 0;             // |Ψ*| tras la fila
    std::vector<domain::ColumnSet> testors;  // Ψ* (orden canónico); vacío si se omitió
    bool testorsOmitted = false;             // true si se superó el límite de instantáneas
    std::chrono::nanoseconds cumulative{0};  // tiempo acumulado del algoritmo
};

struct YycRunResult {
    std::vector<domain::ColumnSet> typicalTestors;  // orden canónico, siempre completo
    std::vector<YycRowRecord> rows;
    std::chrono::nanoseconds total{0};
    std::size_t repetitions = 1;
};

struct YycRunOptions {
    domain::YycOptions solver;
    // Máximo de conjuntos guardados entre todas las instantáneas por fila.
    // Evita un consumo de memoria O(m·|Ψ*|) en matrices grandes; superado el
    // límite sólo se guarda |Ψ*| de cada fila.
    std::size_t maxSnapshotSets = 200000;
};

// Ejercicio 2.3: ejecuta YYC midiendo el tiempo acumulado después de cada fila.
// Sólo se cronometra el trabajo del algoritmo; la notificación al listener, la
// copia de Ψ* para el registro y la impresión quedan fuera de la medición.
// `listener` puede ser nullptr.
YycRunResult runYyc(const domain::BooleanMatrix& matrix, Clock& clock, YycProgressListener* listener,
                    const YycRunOptions& options = {});

// Repite la ejecución `repetitions` veces, toma para cada fila la mediana de
// su tiempo de procesamiento y acumula esas medianas. Reduce el ruido del
// sistema operativo en matrices pequeñas, donde una corrida dura microsegundos.
YycRunResult benchmarkYyc(const domain::BooleanMatrix& matrix, Clock& clock, std::size_t repetitions,
                          const YycRunOptions& options = {});

}  // namespace testores::application
