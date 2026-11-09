#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "testores/domain/column_set.hpp"
#include "testores/domain/yyc.hpp"

// Puertos (interfaces) que la capa de aplicación necesita del exterior.
// Las implementaciones concretas viven en infrastructure/ o presentation/.
namespace testores::application {

class RandomSource {
public:
    virtual ~RandomSource() = default;
    // Devuelve true con probabilidad p (0 <= p <= 1).
    virtual bool bernoulli(double p) = 0;
    virtual std::uint64_t seed() const noexcept = 0;
};

class Clock {
public:
    virtual ~Clock() = default;
    virtual std::chrono::nanoseconds now() = 0;
};

struct YycRowProgress {
    std::size_t rowIndex;   // base 0
    std::size_t totalRows;
    const std::vector<domain::ColumnSet>& current;  // Ψ* tras la fila (orden interno de YYC)
    std::chrono::nanoseconds cumulative;           // tiempo acumulado del algoritmo
    const domain::YycStepStats& stats;
    const domain::YycStepTrace& trace;  // vacía si no se pidió traza
};

class YycProgressListener {
public:
    virtual ~YycProgressListener() = default;
    virtual void onRowProcessed(const YycRowProgress& progress) = 0;
};

}  // namespace testores::application
