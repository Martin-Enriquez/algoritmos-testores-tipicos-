#pragma once

#include <algorithm>
#include <chrono>
#include <vector>

namespace testores::application::detail {

// Mediana convencional: con un número par de muestras se promedian los dos
// valores centrales. La diferencia evita sumar dos duraciones grandes.
// Se trunca como máximo medio nanosegundo por la resolución del tipo.
inline std::chrono::nanoseconds medianDuration(std::vector<std::chrono::nanoseconds>& samples) {
    auto upper = samples.begin() + static_cast<std::ptrdiff_t>(samples.size() / 2);
    std::nth_element(samples.begin(), upper, samples.end());
    if (samples.size() % 2 != 0) {
        return *upper;
    }
    const auto lower = *std::max_element(samples.begin(), upper);
    return lower + (*upper - lower) / 2;
}

}  // namespace testores::application::detail
