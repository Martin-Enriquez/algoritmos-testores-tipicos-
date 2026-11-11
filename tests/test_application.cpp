#include <chrono>
#include <cmath>
#include <stdexcept>

#include "test_framework.hpp"
#include "testores/application/matrix_generation.hpp"
#include "testores/application/bt_runner.hpp"
#include "testores/application/yyc_runner.hpp"
#include "testores/domain/basic_matrix.hpp"
#include "testores/infrastructure/system_adapters.hpp"

using namespace testores;
using application::GenerationRequest;

namespace {

// Reloj falso: avanza 1 µs en cada lectura, lo que hace los tiempos deterministas.
class FakeClock final : public application::Clock {
public:
    std::chrono::nanoseconds now() override { return ticks_ += std::chrono::microseconds(1); }

private:
    std::chrono::nanoseconds ticks_{0};
};

class RecordingListener final : public application::YycProgressListener {
public:
    void onRowProcessed(const application::YycRowProgress& progress) override {
        rows.push_back(progress.rowIndex);
        sizes.push_back(progress.current.size());
    }
    std::vector<std::size_t> rows;
    std::vector<std::size_t> sizes;
};

class DurationClock final : public application::Clock {
public:
    explicit DurationClock(std::vector<long long> durations) : durations_(std::move(durations)) {}
    std::chrono::nanoseconds now() override {
        if ((reads_++ % 2) == 1) {
            ticks_ += std::chrono::nanoseconds(durations_.at(next_++));
        }
        return ticks_;
    }
private:
    std::vector<long long> durations_;
    std::size_t next_ = 0;
    std::size_t reads_ = 0;
    std::chrono::nanoseconds ticks_{0};
};

}  // namespace

TEST(generation_has_requested_shape_and_supports_100x10) {
    infrastructure::MersenneRandomSource random(1);
    const auto m = application::generateRandomMatrix({100, 10, 0.5}, random);
    CHECK_EQ(m.rows(), 100u);
    CHECK_EQ(m.cols(), 10u);
    CHECK(m.density() > 0.35 && m.density() < 0.65);
}

TEST(generation_is_reproducible_with_seed) {
    infrastructure::MersenneRandomSource a(42);
    infrastructure::MersenneRandomSource b(42);
    infrastructure::MersenneRandomSource c(43);
    const auto ma = application::generateRandomMatrix({30, 8, 0.5}, a);
    CHECK(ma == application::generateRandomMatrix({30, 8, 0.5}, b));
    CHECK(!(ma == application::generateRandomMatrix({30, 8, 0.5}, c)));
}

TEST(generation_probability_extremes) {
    infrastructure::MersenneRandomSource random(5);
    CHECK_EQ(application::generateRandomMatrix({5, 5, 0.0}, random).onesCount(), 0u);
    CHECK_EQ(application::generateRandomMatrix({5, 5, 1.0}, random).onesCount(), 25u);
}

TEST(generation_rejects_invalid_requests) {
    infrastructure::MersenneRandomSource random(5);
    CHECK_THROWS(application::generateRandomMatrix({0, 5, 0.5}, random), std::invalid_argument);
    CHECK_THROWS(application::generateRandomMatrix({5, 0, 0.5}, random), std::invalid_argument);
    CHECK_THROWS(application::generateRandomMatrix({5, 5, 1.5}, random), std::invalid_argument);
    CHECK_THROWS(application::generateRandomMatrix({5, 5, std::nan("")}, random), std::invalid_argument);
    CHECK_THROWS(application::generateRandomMatrix({domain::MatrixLimits::kMaxRows + 1, 5, 0.5}, random),
                 std::invalid_argument);
}

TEST(generate_with_basic_returns_consistent_reduction) {
    infrastructure::MersenneRandomSource random(11);
    const auto g = application::generateWithBasic({40, 7, 0.5}, random);
    CHECK_EQ(g.reduction.decisions.size(), 40u);
    CHECK(domain::isBasicMatrix(g.reduction.basic));
    CHECK(g.reduction.basic == domain::toBasicMatrix(g.original));
}

TEST(search_finds_basic_matrix_meeting_constraints) {
    infrastructure::MersenneRandomSource random(2026);
    application::BasicMatrixConstraints constraints;
    constraints.minDensity = 0.4;
    constraints.minRows = 4;
    const auto outcome = application::searchBasicMatrix({8, 7, 0.5}, constraints, random);
    CHECK(outcome.found.has_value());
    const auto& basic = outcome.found->reduction.basic;
    CHECK(basic.cols() >= 7);
    CHECK(basic.density() >= 0.4);
    CHECK(basic.rows() >= 4);
    CHECK(domain::isBasicMatrix(basic));
    CHECK(application::satisfies(basic, constraints));
}

TEST(search_reports_failure_when_impossible) {
    infrastructure::MersenneRandomSource random(3);
    application::BasicMatrixConstraints constraints;
    constraints.minRows = 5;  // con 2 filas generadas es imposible
    constraints.maxAttempts = 50;
    const auto outcome = application::searchBasicMatrix({2, 4, 0.5}, constraints, random);
    CHECK(!outcome.found.has_value());
    CHECK_EQ(outcome.attempts, 50u);
}

TEST(search_rejects_invalid_constraints_before_generating) {
    infrastructure::MersenneRandomSource random(3);
    application::BasicMatrixConstraints constraints;
    constraints.minDensity = std::nan("");
    CHECK_THROWS(application::searchBasicMatrix({2, 4, 0.5}, constraints, random), std::invalid_argument);
    constraints.minDensity = 0.4;
    constraints.minRows = 0;
    CHECK_THROWS(application::searchBasicMatrix({2, 4, 0.5}, constraints, random), std::invalid_argument);
    constraints.minRows = 1;
    constraints.maxAttempts = 0;
    CHECK_THROWS(application::searchBasicMatrix({2, 4, 0.5}, constraints, random), std::invalid_argument);
}

TEST(run_yyc_reports_each_row_with_monotonic_cumulative_time) {
    const auto m = domain::BooleanMatrix::fromGrid({{1, 1, 0}, {0, 1, 1}, {1, 0, 1}});
    FakeClock clock;
    RecordingListener listener;
    const auto result = application::runYyc(m, clock, &listener);
    CHECK(listener.rows == (std::vector<std::size_t>{0, 1, 2}));
    CHECK(listener.sizes == (std::vector<std::size_t>{2, 2, 3}));
    CHECK_EQ(result.rows.size(), 3u);
    CHECK_EQ(result.typicalTestors.size(), 3u);
    // El reloj falso avanza 1 µs por lectura: 2 lecturas por fila = 1 µs por fila.
    CHECK(result.rows[0].cumulative == std::chrono::microseconds(1));
    CHECK(result.rows[2].cumulative == std::chrono::microseconds(3));
    CHECK(result.total == result.rows.back().cumulative);
}

TEST(run_yyc_omits_snapshots_beyond_limit_but_keeps_counts_and_result) {
    const auto m = domain::BooleanMatrix::fromGrid({{1, 1, 0}, {0, 1, 1}, {1, 0, 1}});
    FakeClock clock;
    application::YycRunOptions options;
    options.maxSnapshotSets = 4;  // r1 (2) + r2 (2) caben; r3 (3) no
    const auto result = application::runYyc(m, clock, nullptr, options);
    CHECK(!result.rows[1].testorsOmitted);
    CHECK(result.rows[2].testorsOmitted);
    CHECK(result.rows[2].testors.empty());
    CHECK_EQ(result.rows[2].testorCount, 3u);
    CHECK_EQ(result.typicalTestors.size(), 3u);
}

TEST(benchmark_yyc_reports_the_median_of_the_cumulative_time) {
    // Reloj programado: duraciones (µs) de las filas r1 y r2 en tres corridas.
    //   corrida 1: 1 y 10  -> acumulados 1 y 11
    //   corrida 2: 10 y 1  -> acumulados 10 y 11
    //   corrida 3: 5 y 5   -> acumulados 5 y 10
    // Mediana del acumulado: r1 = 5, r2 = 11. Sumar las medianas por fila daría
    // r2 = 5 + 5 = 10, que no es el acumulado de ninguna corrida.
    class ScriptedClock final : public application::Clock {
    public:
        explicit ScriptedClock(std::vector<long> durations) : durations_(std::move(durations)) {}
        std::chrono::nanoseconds now() override {
            // Lecturas impares: inicio de fila; pares: fin de fila (inicio + duración).
            const bool end = (reads_++ % 2) == 1;
            if (end) now_ += std::chrono::microseconds(durations_.at(next_++));
            else now_ += std::chrono::microseconds(100);  // tiempo fuera de la medición
            return now_;
        }

    private:
        std::vector<long> durations_;
        std::size_t next_ = 0;
        std::size_t reads_ = 0;
        std::chrono::nanoseconds now_{0};
    };
    const auto m = domain::BooleanMatrix::fromGrid({{1, 1, 0}, {0, 1, 1}});
    ScriptedClock clock({1, 10, 10, 1, 5, 5});
    const auto result = application::benchmarkYyc(m, clock, 3);
    CHECK(result.rows[0].cumulative == std::chrono::microseconds(5));
    CHECK(result.rows[1].cumulative == std::chrono::microseconds(11));
    CHECK(result.total == std::chrono::microseconds(11));
}

TEST(benchmark_yyc_uses_median_and_keeps_results) {
    const auto m = domain::BooleanMatrix::fromGrid({{1, 1, 0}, {0, 1, 1}, {1, 0, 1}});
    FakeClock clock;
    const auto result = application::benchmarkYyc(m, clock, 5);
    CHECK_EQ(result.repetitions, 5u);
    CHECK_EQ(result.typicalTestors.size(), 3u);
    CHECK(result.rows[0].cumulative <= result.rows[1].cumulative);
    CHECK(result.rows[1].cumulative <= result.rows[2].cumulative);
    CHECK_THROWS(application::benchmarkYyc(m, clock, 0), std::invalid_argument);
    CHECK_THROWS(application::benchmarkYyc(m, clock, 10000001), std::invalid_argument);
}

TEST(benchmarks_average_the_two_central_samples_for_even_repetitions) {
    const auto m = domain::BooleanMatrix::fromGrid({{1, 1, 0}, {0, 1, 1}});
    DurationClock yycClock({1000, 10000, 10000, 1000, 5000, 5000, 7000, 1000});
    const auto yyc = application::benchmarkYyc(m, yycClock, 4);
    CHECK(yyc.rows[0].cumulative == std::chrono::nanoseconds(6000));
    CHECK(yyc.rows[1].cumulative == std::chrono::nanoseconds(10500));
    CHECK(yyc.total == yyc.rows.back().cumulative);
    DurationClock btClock({1000, 9000, 3000, 5000});
    const auto bt = application::benchmarkBt(m, btClock, 4);
    CHECK(bt.total == std::chrono::nanoseconds(4000));
    CHECK(bt.typicalityProceduresAgree);
}
