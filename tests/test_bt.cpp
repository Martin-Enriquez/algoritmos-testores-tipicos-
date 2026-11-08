#include <algorithm>
#include <chrono>
#include <cstdint>
#include <random>
#include <sstream>
#include <stdexcept>

#include "test_framework.hpp"
#include "testores/application/bt_runner.hpp"
#include "testores/domain/basic_matrix.hpp"
#include "testores/domain/boolean_matrix.hpp"
#include "testores/domain/bt.hpp"
#include "testores/domain/notation.hpp"
#include "testores/domain/testor.hpp"
#include "testores/domain/yyc.hpp"
#include "testores/infrastructure/result_export.hpp"
#include "testores/presentation/cli.hpp"

using namespace testores;
using namespace testores::domain;

namespace {

// Oráculo exhaustivo independiente de BT, YYC y domain/testor: recorre los
// 2^n subconjuntos (bit j de la máscara = columna j) aplicando las definiciones.
bool isTestorMask(const std::vector<std::vector<int>>& grid, unsigned mask) {
    for (const auto& row : grid) {
        bool covered = false;
        for (std::size_t j = 0; j < row.size(); ++j) {
            covered = covered || (row[j] == 1 && ((mask >> j) & 1U) != 0);
        }
        if (!covered) return false;
    }
    return true;
}

bool isTypicalMask(const std::vector<std::vector<int>>& grid, unsigned mask) {
    if (mask == 0 || !isTestorMask(grid, mask)) return false;
    for (std::size_t j = 0; j < grid.front().size(); ++j) {
        if (((mask >> j) & 1U) != 0 && isTestorMask(grid, mask & ~(1U << j))) return false;
    }
    return true;
}

ColumnSet fromMask(std::size_t n, unsigned mask) {
    ColumnSet s(n);
    for (std::size_t j = 0; j < n; ++j) {
        if (((mask >> j) & 1U) != 0) s.set(j);
    }
    return s;
}

std::vector<ColumnSet> bruteForceTypicalTestors(const std::vector<std::vector<int>>& grid) {
    const std::size_t n = grid.front().size();
    std::vector<ColumnSet> result;
    for (unsigned mask = 1; mask < (1U << n); ++mask) {
        if (isTypicalMask(grid, mask)) result.push_back(fromMask(n, mask));
    }
    std::sort(result.begin(), result.end(), canonicalLess);
    return result;
}

std::vector<std::vector<int>> randomGrid(std::mt19937& rng, std::size_t rows, std::size_t cols, double p) {
    std::bernoulli_distribution bit(p);
    std::vector<std::vector<int>> grid(rows, std::vector<int>(cols));
    for (auto& row : grid) {
        for (auto& v : row) v = bit(rng) ? 1 : 0;
    }
    return grid;
}

// Matriz A del reporte (Ejercicio 2.1).
BooleanMatrix matrixA() {
    return BooleanMatrix::fromGrid({{1, 0, 1, 0, 1, 0, 1},
                                    {1, 0, 0, 0, 1, 1, 0},
                                    {1, 1, 0, 1, 0, 0, 0},
                                    {0, 0, 1, 1, 0, 1, 1},
                                    {1, 0, 0, 1, 0, 1, 0},
                                    {1, 0, 0, 1, 0, 0, 1}});
}

ColumnSet bits(const std::string& binary) {
    ColumnSet s(binary.size());
    for (std::size_t j = 0; j < binary.size(); ++j) s.set(j, binary[j] == '1');
    return s;
}

class FakeClock final : public application::Clock {
public:
    std::chrono::nanoseconds now() override { return ticks_ += std::chrono::microseconds(1); }

private:
    std::chrono::nanoseconds ticks_{0};
};

}  // namespace

TEST(bt_number_uses_x1_as_most_significant_bit) {
    CHECK(btNumber(bits("0000001")) == std::optional<std::uint64_t>(1));
    CHECK(btNumber(bits("1000000")) == std::optional<std::uint64_t>(64));
    CHECK(btNumber(bits("1010000")) == std::optional<std::uint64_t>(80));
    CHECK(!btNumber(ColumnSet(64)).has_value());
}

TEST(column_set_last_index) {
    CHECK_EQ(ColumnSet(7).lastIndex(), 7u);  // vacío: devuelve width()
    CHECK_EQ(bits("1010000").lastIndex(), 2u);
    CHECK_EQ(ColumnSet::fromIndices(200, {3, 64, 130}).lastIndex(), 130u);
    CHECK_EQ(ColumnSet::fromIndices(512, {0, 511}).lastIndex(), 511u);
}

TEST(bt_jumps_follow_binary_arithmetic) {
    // S1: 1 en la columna pivote y ceros a su derecha.
    CHECK(btJumpNotTestor(bits("0101"), 2) == bits("0110"));
    CHECK(btJumpNotTestor(bits("0001"), 0) == bits("1000"));
    CHECK_THROWS(btJumpNotTestor(bits("0110"), 2), std::invalid_argument);
    // S2: sumar 1 en la posición del último 1 (con acarreo).
    CHECK(*btJumpTestor(bits("0101")) == bits("0110"));
    CHECK(*btJumpTestor(bits("0110")) == bits("1000"));
    CHECK(*btJumpTestor(bits("1011")) == bits("1100"));
    CHECK(!btJumpTestor(bits("1100")).has_value());  // 1100 + 0100 desborda
    CHECK(!btJumpTestor(bits("1111")).has_value());
}

TEST(bt_trace_on_small_example_is_exact) {
    // r1 = 110, r2 = 011, r3 = 101.
    BtOptions options;
    options.recordTrace = true;
    const auto result = BtSolver::solve(BooleanMatrix::fromGrid({{1, 1, 0}, {0, 1, 1}, {1, 0, 1}}), options);
    const std::vector<std::string> visited = {"001", "010", "011", "100", "101", "110"};
    CHECK_EQ(result.trace.size(), visited.size());
    for (std::size_t s = 0; s < visited.size(); ++s) CHECK(result.trace[s].candidate == bits(visited[s]));

    CHECK(result.trace[0].verdict == BtVerdict::NotTestor);
    CHECK_EQ(result.trace[0].zeroRow, 0u);  // r1 = 110 no corta a {x3}
    CHECK_EQ(result.trace[0].pivot, 1u);    // último 1 de r1: x2
    CHECK(result.trace[1].verdict == BtVerdict::NotTestor);
    CHECK_EQ(result.trace[1].zeroRow, 2u);  // r3 = 101 no corta a {x2}
    CHECK(result.trace[2].verdict == BtVerdict::TypicalTestor);
    CHECK(result.trace[3].verdict == BtVerdict::NotTestor);
    CHECK(result.trace[4].verdict == BtVerdict::TypicalTestor);
    CHECK(result.trace[5].verdict == BtVerdict::TypicalTestor);
    CHECK(!result.trace[5].next.has_value());  // 110 + 010 desborda: fin

    CHECK_EQ(result.stats.candidatesEvaluated, 6u);
    CHECK_EQ(result.typicalTestors.size(), 3u);
}

TEST(bt_finds_the_eight_typical_testors_of_matrix_a) {
    const auto result = BtSolver::solve(matrixA());
    CHECK_EQ(formatFeatureSets(result.typicalTestors),
             std::string("{x1, x3}, {x1, x4}, {x1, x6}, {x1, x7}, {x4, x5}, {x2, x6, x7}, {x3, x4, x6}, {x4, x6, x7}"));
    CHECK(result.typicalTestors == YycSolver::solve(matrixA()));
    CHECK(result.typicalTestors == bruteForceTypicalTestors(matrixA().toGrid()));
}

TEST(bt_matches_brute_force_and_yyc_on_random_basic_matrices) {
    std::mt19937 rng(20261108);
    int checked = 0;
    for (int trial = 0; trial < 500; ++trial) {
        const std::size_t cols = 1 + static_cast<std::size_t>(trial % 12);  // 1..12
        const std::size_t rows = 1 + static_cast<std::size_t>(trial % 19);
        const double p = 0.15 + 0.1 * static_cast<double>(trial % 7);
        const BooleanMatrix basic = toBasicMatrix(BooleanMatrix::fromGrid(randomGrid(rng, rows, cols, p)));
        if (basic.rows() == 0) continue;
        const auto bt = BtSolver::solve(basic);
        CHECK(bt.typicalTestors == bruteForceTypicalTestors(basic.toGrid()));
        CHECK(bt.typicalTestors == YycSolver::solve(basic));
        ++checked;
    }
    CHECK(checked > 400);
}

TEST(bt_visits_in_increasing_order_and_only_skips_non_typical_tuples) {
    // Comprueba las proposiciones de salto: todo n-uplo que BT no analiza
    // no es testor típico, y todo testor analizado se clasificó bien.
    std::mt19937 rng(31337);
    for (int trial = 0; trial < 150; ++trial) {
        const std::size_t cols = 2 + static_cast<std::size_t>(trial % 9);  // 2..10
        const std::size_t rows = 2 + static_cast<std::size_t>(trial % 13);
        const BooleanMatrix basic =
            toBasicMatrix(BooleanMatrix::fromGrid(randomGrid(rng, rows, cols, 0.2 + 0.1 * (trial % 5))));
        if (basic.rows() == 0) continue;
        BtOptions options;
        options.recordTrace = true;
        const auto result = BtSolver::solve(basic, options);
        const auto grid = basic.toGrid();

        std::vector<bool> visited(std::size_t{1} << cols, false);
        std::uint64_t previous = 0;
        for (const auto& step : result.trace) {
            const std::uint64_t value = *btNumber(step.candidate);
            CHECK(value > previous);  // orden estrictamente creciente
            previous = value;
            unsigned mask = 0;
            for (std::size_t j = 0; j < cols; ++j) {
                if (step.candidate.test(j)) mask |= 1U << j;
            }
            visited[mask] = true;
            const bool testor = isTestorMask(grid, mask);
            CHECK_EQ(step.verdict != BtVerdict::NotTestor, testor);
            if (testor) CHECK_EQ(step.verdict == BtVerdict::TypicalTestor, isTypicalMask(grid, mask));
            if (!testor) CHECK(!basic.row(step.zeroRow).intersects(step.candidate));
        }
        for (unsigned mask = 1; mask < (1U << cols); ++mask) {
            if (!visited[mask]) CHECK(!isTypicalMask(grid, mask));
        }
        CHECK_EQ(result.stats.candidatesEvaluated, result.trace.size());
    }
}

TEST(bt_minimal_testors_found_are_the_typical_ones) {
    std::mt19937 rng(4242);
    for (int trial = 0; trial < 100; ++trial) {
        const BooleanMatrix basic = toBasicMatrix(BooleanMatrix::fromGrid(randomGrid(rng, 10, 8, 0.35)));
        if (basic.rows() == 0) continue;
        const auto result = BtSolver::solve(basic);
        std::vector<ColumnSet> found;
        for (const auto& t : result.testors) {
            found.push_back(t.set);
            CHECK(isTestor(basic, t.set));
            CHECK_EQ(t.typical, isTypicalTestor(basic, t.set));
        }
        CHECK(minimalSets(found) == result.typicalTestors);
    }
}

TEST(bt_result_does_not_depend_on_row_order_but_trace_does) {
    const BooleanMatrix a = matrixA();
    auto rows = a.rowSets();
    std::sort(rows.begin(), rows.end(), [](const ColumnSet& x, const ColumnSet& y) { return x.count() < y.count(); });
    const BooleanMatrix sorted(a.cols(), rows);
    CHECK(BtSolver::solve(a).typicalTestors == BtSolver::solve(sorted).typicalTestors);
}

TEST(bt_rejects_empty_matrix_and_zero_rows) {
    CHECK_THROWS(BtSolver::solve(BooleanMatrix()), std::invalid_argument);
    CHECK_THROWS(BtSolver::solve(BooleanMatrix::fromGrid({{1, 0}, {0, 0}})), std::invalid_argument);
}

TEST(bt_single_column_and_resource_limit) {
    const auto single = BtSolver::solve(BooleanMatrix::fromGrid({{1}, {1}}));
    CHECK_EQ(single.typicalTestors.size(), 1u);
    CHECK_EQ(single.stats.candidatesEvaluated, 1u);

    // Filas disjuntas de 2 unos: 2^6 = 64 testores típicos.
    std::vector<std::vector<int>> grid;
    for (std::size_t i = 0; i < 6; ++i) {
        std::vector<int> row(12, 0);
        row[2 * i] = row[2 * i + 1] = 1;
        grid.push_back(row);
    }
    BtOptions options;
    options.maxTypicalTestors = 10;
    CHECK_THROWS(BtSolver::solve(BooleanMatrix::fromGrid(grid), options), BtResourceLimitExceeded);
    CHECK_EQ(BtSolver::solve(BooleanMatrix::fromGrid(grid)).typicalTestors.size(), 64u);
}

TEST(run_bt_measures_total_time_and_checks_typicality_both_ways) {
    FakeClock clock;
    const auto run = application::runBt(matrixA(), clock);
    CHECK(run.total == std::chrono::microseconds(1));  // una lectura antes y otra después
    CHECK(run.typicalityProceduresAgree);
    CHECK(run.typicalByMinimality == run.run.typicalTestors);

    const auto bench = application::benchmarkBt(matrixA(), clock, 5);
    CHECK_EQ(bench.repetitions, 5u);
    CHECK(bench.total == std::chrono::microseconds(1));
    CHECK(bench.run.typicalTestors == run.run.typicalTestors);
    CHECK_THROWS(application::benchmarkBt(matrixA(), clock, 0), std::invalid_argument);
}

TEST(bt_exports) {
    FakeClock clock;
    BtOptions options;
    options.recordTrace = true;
    const auto run = application::runBt(matrixA(), clock, options);
    std::ostringstream csv;
    infrastructure::writeBtCsv(csv, run);
    CHECK(csv.str().find("orden,alfa,testor,tipico") == 0);
    CHECK(csv.str().find("\"{x4, x5}\",si") != std::string::npos);
    std::ostringstream summary;
    infrastructure::writeBtSummaryLatex(summary, matrixA(), run);
    CHECK(summary.str().find("Testores típicos") != std::string::npos);
    std::ostringstream trace;
    infrastructure::writeBtTraceLatex(trace, matrixA(), run.run);
    CHECK(trace.str().find("x_{4}{:}r_{3}") != std::string::npos);
    CHECK(trace.str().find("fin") != std::string::npos);
}

TEST(interactive_session_runs_bt) {
    // 1: generar 6x5 (semilla 3); 10: BT con 1 repetición; 11: BT paso a paso; 0: salir.
    std::istringstream in("1\n6\n5\n\n3\n10\n1\n11\n0\n");
    std::ostringstream out;
    std::ostringstream err;
    CHECK_EQ(presentation::runApplication({}, in, out, err), 0);
    CHECK(out.str().find("Testores típicos de la matriz") != std::string::npos);
    CHECK(out.str().find("Tiempo total de ejecución") != std::string::npos);
    CHECK(out.str().find("Ambos procedimientos coinciden") != std::string::npos);
    CHECK(out.str().find("Paso 1: α = 00001") != std::string::npos);
}
