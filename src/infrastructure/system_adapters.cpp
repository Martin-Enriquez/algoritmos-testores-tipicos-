#include "testores/infrastructure/system_adapters.hpp"

#include <stdexcept>

namespace testores::infrastructure {

MersenneRandomSource::MersenneRandomSource(std::uint64_t seed) : seed_(seed), engine_(seed) {}

std::uint64_t MersenneRandomSource::freshSeed() {
    std::random_device device;
    return (static_cast<std::uint64_t>(device()) << 32) ^ static_cast<std::uint64_t>(device());
}

bool MersenneRandomSource::bernoulli(double p) {
    if (!(p >= 0.0 && p <= 1.0)) {
        throw std::invalid_argument("La probabilidad debe estar en [0, 1]");
    }
    // Uniforme en [0, 1) con 53 bits a partir de mt19937_64, cuya secuencia está
    // fijada por el estándar. Se evita std::bernoulli_distribution porque su
    // algoritmo depende de la implementación y rompería la reproducibilidad
    // entre compiladores (g++, clang, MSVC).
    const double u = static_cast<double>(engine_() >> 11) * 0x1.0p-53;
    return u < p;
}

std::chrono::nanoseconds SteadyClock::now() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch());
}

}  // namespace testores::infrastructure
