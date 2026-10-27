#pragma once

#include <chrono>
#include <cstdint>
#include <random>

#include "testores/application/ports.hpp"

namespace testores::infrastructure {

// Generador Mersenne Twister de 64 bits. Con la misma semilla produce
// exactamente la misma secuencia, lo que hace reproducibles los experimentos.
class MersenneRandomSource final : public application::RandomSource {
public:
    explicit MersenneRandomSource(std::uint64_t seed);

    // Semilla no determinista obtenida de std::random_device.
    static std::uint64_t freshSeed();

    bool bernoulli(double p) override;
    std::uint64_t seed() const noexcept override { return seed_; }

private:
    std::uint64_t seed_;
    std::mt19937_64 engine_;
};

// Reloj monótono: no retrocede si se ajusta la hora del sistema.
class SteadyClock final : public application::Clock {
public:
    std::chrono::nanoseconds now() override;
};

}  // namespace testores::infrastructure
