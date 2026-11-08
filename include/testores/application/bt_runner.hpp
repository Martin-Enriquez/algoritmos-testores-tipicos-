#pragma once

#include <chrono>
#include <cstddef>
#include <vector>

#include "testores/application/ports.hpp"
#include "testores/domain/boolean_matrix.hpp"
#include "testores/domain/bt.hpp"
#include "testores/domain/column_set.hpp"

namespace testores::application {

struct BtRunResult {
    domain::BtResult run;
    std::chrono::nanoseconds total{0};  // tiempo total del algoritmo
    std::size_t repetitions = 1;
    // Procedimiento a posteriori (Ej. 3.2, punto 2): los testores minimales
    // entre los encontrados. Se calcula fuera de la medición de tiempo y debe
    // coincidir con los típicos detectados durante la ejecución.
    std::vector<domain::ColumnSet> typicalByMinimality;
    bool typicalityProceduresAgree = false;
};

// Ejercicio 3.2: ejecuta BT midiendo su tiempo total con el reloj dado.
BtRunResult runBt(const domain::BooleanMatrix& matrix, Clock& clock, const domain::BtOptions& options = {});

// Repite BT `repetitions` veces (sin traza) y reporta la mediana del tiempo
// total. El resultado (testores) es el de la primera ejecución con `options`.
BtRunResult benchmarkBt(const domain::BooleanMatrix& matrix, Clock& clock, std::size_t repetitions,
                        const domain::BtOptions& options = {});

}  // namespace testores::application
