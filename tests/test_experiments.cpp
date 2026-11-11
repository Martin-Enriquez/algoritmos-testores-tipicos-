#include <chrono>
#include <stdexcept>
#include <utility>
#include <vector>

#include "test_framework.hpp"
#include "testores/application/experiments.hpp"

using namespace testores;

namespace {

class DurationClock final : public application::Clock {
public:
    explicit DurationClock(std::vector<long long> durations) : durations_(std::move(durations)) {}
    std::chrono::nanoseconds now() override {
        if (reads_++ % 2 == 1) ticks_ += std::chrono::nanoseconds(durations_.at(next_++));
        return ticks_;
    }
    std::size_t consumed() const { return next_; }
private:
    std::vector<long long> durations_;
    std::chrono::nanoseconds ticks_{0};
    std::size_t reads_ = 0;
    std::size_t next_ = 0;
};

class StepClock final : public application::Clock {
public:
    std::chrono::nanoseconds now() override {
        return std::chrono::nanoseconds(static_cast<long long>(reads_++) * 10);
    }
    std::size_t reads() const { return reads_; }
private:
    std::size_t reads_ = 0;
};

class SequenceClock final : public application::Clock {
public:
    explicit SequenceClock(std::vector<long long> values) : values_(std::move(values)) {}
    std::chrono::nanoseconds now() override { return std::chrono::nanoseconds(values_.at(next_++)); }
private:
    std::vector<long long> values_;
    std::size_t next_ = 0;
};

class RecordingProgress final : public application::ExperimentProgress {
public:
    void onMeasurement(const application::ExperimentCase&, std::size_t index,
                       const application::ExperimentMeasurement& measurement) override {
        indices.push_back(index);
        statuses.push_back(measurement.status);
    }
    std::vector<std::size_t> indices;
    std::vector<std::string> statuses;
};

application::ExperimentCase smallExperiment() {
    return {"manual", 1, domain::BooleanMatrix::fromGrid({{1,0},{0,1}}), 1};
}

}  // namespace

TEST(experiment_warmup_is_excluded_and_all_four_methods_use_the_even_median) {
    std::vector<long long> durations;
    for (int method = 0; method < 4; ++method) {
        // Calentamiento 9000 ns; muestras 100, 300, 500, 700 -> mediana 400 ns.
        durations.insert(durations.end(), {9000, 100, 300, 500, 700});
    }
    DurationClock clock(std::move(durations));
    application::ExperimentOptions options;
    options.repetitions = 4;
    options.timeLimit = std::chrono::nanoseconds{0};
    RecordingProgress progress;
    const auto results = application::measureExperiment(smallExperiment(), clock, options, &progress);
    for (const auto& result : results) {
        CHECK_EQ(result.status, std::string("ok"));
        CHECK_EQ(result.completedRuns, 4U);
        CHECK(result.median == std::chrono::nanoseconds(400));
    }
    CHECK(results[2].candidates == results[3].candidates);
    CHECK_EQ(clock.consumed(), 20U);
    CHECK(progress.indices == (std::vector<std::size_t>{0,1,2,3}));
}

TEST(experiment_cancellation_at_the_exact_limit_never_reports_a_partial_median) {
    StepClock clock;
    application::ExperimentOptions options;
    options.repetitions = 3;
    options.timeLimit = std::chrono::nanoseconds(10);
    RecordingProgress progress;
    const auto results = application::measureExperiment(smallExperiment(), clock, options, &progress);
    for (const auto& result : results) {
        CHECK_EQ(result.status, std::string("limite"));
        CHECK_EQ(result.completedRuns, 0U);
        CHECK(result.median == std::chrono::nanoseconds{0});
        CHECK(!result.detail.empty());
    }
    // Inicio y primera consulta de cancelación por método. Distingue >= de >.
    CHECK_EQ(clock.reads(), 8U);
    CHECK_EQ(progress.statuses.size(), 4U);
}

TEST(experiment_checks_the_limit_after_a_short_solver_finishes) {
    // Una única fila/candidato sólo consulta una vez la cancelación interna.
    // El límite se alcanza después de esa consulta, al terminar solve.
    SequenceClock clock({0,0,10, 20,20,30, 40,40,50, 60,60,70});
    application::ExperimentOptions options;
    options.repetitions = 1;
    options.timeLimit = std::chrono::nanoseconds(10);
    const application::ExperimentCase experiment{
        "manual", 1, domain::BooleanMatrix::fromGrid({{1}}), 1};
    const auto results = application::measureExperiment(experiment, clock, options);
    for (const auto& result : results) {
        CHECK_EQ(result.status, std::string("limite"));
        CHECK_EQ(result.completedRuns, 0U);
    }
}

TEST(experiment_rejects_bad_policies_and_wrong_theoretical_cardinalities) {
    StepClock clock;
    application::ExperimentOptions options;
    options.repetitions = 0;
    CHECK_THROWS(application::measureExperiment(smallExperiment(), clock, options), std::invalid_argument);
    options.repetitions = 10001;
    CHECK_THROWS(application::measureExperiment(smallExperiment(), clock, options), std::invalid_argument);
    options.repetitions = 1;
    options.timeLimit = std::chrono::nanoseconds(-1);
    CHECK_THROWS(application::measureExperiment(smallExperiment(), clock, options), std::invalid_argument);
    options.timeLimit = std::chrono::nanoseconds{0};
    auto wrong = smallExperiment();
    wrong.expectedTypical = 2;
    CHECK_THROWS(application::measureExperiment(wrong, clock, options), std::logic_error);
}

TEST(experiment_cases_match_the_complete_project_tables_and_duality) {
    const auto a = domain::BooleanMatrix::fromGrid({{1,0,1,0,1,0,1},{1,0,0,0,1,1,0},
        {1,1,0,1,0,0,0},{0,0,1,1,0,1,1},{1,0,0,1,0,1,0},{1,0,0,1,0,0,1}});
    const auto b = application::buildDualMatrix(a);
    CHECK_EQ(b.rows(), 8U);
    CHECK_EQ(b.cols(), 7U);
    const auto cases = application::buildExperimentCases(a);
    // C tiene 5 típicos de tamaño 2, 7 de tamaño 3 y 2 de tamaño 4.
    // |TT(phi^N(C))| = 5*N^2 + 7*N^3 + 2*N^4; |TT(gamma^N(C))| = 14^N.
    const std::vector<std::size_t> expected{14,108,396,1040,2250,14,196,2744,38416};
    CHECK_EQ(cases.size(), expected.size());
    for (std::size_t i = 0; i < cases.size(); ++i) {
        CHECK_EQ(cases[i].expectedTypical, expected[i]);
        CHECK_EQ(cases[i].matrix.cols(), 14U * cases[i].copies);
        CHECK_EQ(cases[i].matrix.rows(), i < 5 ? 48U : 48U * cases[i].copies);
    }
    CHECK_THROWS(application::buildDualMatrix(domain::BooleanMatrix{}), std::invalid_argument);
    CHECK_THROWS(application::buildDualMatrix(domain::BooleanMatrix::fromGrid({{0,0}})), std::invalid_argument);
    CHECK_THROWS(application::buildDualMatrix(domain::BooleanMatrix::fromGrid({{1,0},{1,1}})), std::invalid_argument);
}
