#include <algorithm>
#include <random>
#include <stdexcept>

#include "test_framework.hpp"
#include "testores/domain/basic_matrix.hpp"
#include "testores/domain/boolean_matrix.hpp"
#include "testores/domain/testor.hpp"
#include "testores/domain/yyc.hpp"

using namespace testores::domain;

namespace {

// Oráculo exhaustivo independiente de YYC y de domain/testor: recorre los
// 2^n subconjuntos y aplica las definiciones directamente sobre la cuadrícula.
std::vector<ColumnSet> bruteForceTypicalTestors(const std::vector<std::vector<int>>& grid) {
    const std::size_t n = grid.front().size();
    auto isTestorMask = [&](unsigned mask) {
        for (const auto& row : grid) {
            bool covered = false;
            for (std::size_t j = 0; j < n; ++j) {
                covered = covered || (row[j] == 1 && ((mask >> j) & 1U) != 0);
            }
            if (!covered) return false;
        }
        return true;
    };
    std::vector<ColumnSet> result;
    for (unsigned mask = 1; mask < (1U << n); ++mask) {
        if (!isTestorMask(mask)) continue;
        bool minimal = true;
        for (std::size_t j = 0; j < n && minimal; ++j) {
            if (((mask >> j) & 1U) != 0 && isTestorMask(mask & ~(1U << j))) minimal = false;
        }
        if (!minimal) continue;
        ColumnSet s(n);
        for (std::size_t j = 0; j < n; ++j) {
            if (((mask >> j) & 1U) != 0) s.set(j);
        }
        result.push_back(s);
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

}  // namespace

TEST(yyc_identity_matrix_has_single_typical_testor) {
    const auto m = BooleanMatrix::fromGrid({{1, 0, 0}, {0, 1, 0}, {0, 0, 1}});
    const auto result = YycSolver::solve(m);
    CHECK_EQ(result.size(), 1u);
    CHECK(result[0] == ColumnSet::fromIndices(3, {0, 1, 2}));
}

TEST(yyc_single_row_gives_singletons) {
    const auto m = BooleanMatrix::fromGrid({{1, 0, 1, 1}});
    const auto result = YycSolver::solve(m);
    CHECK_EQ(result.size(), 3u);
    for (const auto& t : result) CHECK_EQ(t.count(), 1u);
}

TEST(yyc_hand_example) {
    // r1 = x1 x2, r2 = x2 x3, r3 = x1 x3  ->  TT = {x1,x2}, {x1,x3}, {x2,x3}
    const auto m = BooleanMatrix::fromGrid({{1, 1, 0}, {0, 1, 1}, {1, 0, 1}});
    const auto result = YycSolver::solve(m);
    CHECK_EQ(result.size(), 3u);
    CHECK(result[0] == ColumnSet::fromIndices(3, {0, 1}));
    CHECK(result[1] == ColumnSet::fromIndices(3, {0, 2}));
    CHECK(result[2] == ColumnSet::fromIndices(3, {1, 2}));
}

TEST(yyc_incremental_state_is_typical_testors_of_prefix) {
    std::mt19937 rng(7);
    const auto grid = randomGrid(rng, 12, 8, 0.45);
    const BooleanMatrix basic = toBasicMatrix(BooleanMatrix::fromGrid(grid));
    YycSolver solver(basic);
    while (!solver.finished()) {
        solver.processNextRow();
        std::vector<ColumnSet> prefixRows(basic.rowSets().begin(),
                                          basic.rowSets().begin() + static_cast<std::ptrdiff_t>(solver.processedRows()));
        const BooleanMatrix prefix(basic.cols(), prefixRows);
        auto current = solver.current();
        std::sort(current.begin(), current.end(), canonicalLess);
        CHECK(current == bruteForceTypicalTestors(prefix.toGrid()));
    }
}

TEST(yyc_matches_brute_force_on_random_basic_matrices) {
    std::mt19937 rng(20261027);
    int checked = 0;
    for (int trial = 0; trial < 400; ++trial) {
        const std::size_t cols = 2 + static_cast<std::size_t>(trial % 11);  // 2..12
        const std::size_t rows = 1 + static_cast<std::size_t>(trial % 17);
        const double p = 0.2 + 0.1 * static_cast<double>(trial % 6);
        const BooleanMatrix basic = toBasicMatrix(BooleanMatrix::fromGrid(randomGrid(rng, rows, cols, p)));
        if (basic.rows() == 0) continue;
        const auto yyc = YycSolver::solve(basic);
        CHECK(yyc == bruteForceTypicalTestors(basic.toGrid()));
        for (const auto& t : yyc) CHECK(isTypicalTestor(basic, t));
        ++checked;
    }
    CHECK(checked > 300);
}

TEST(yyc_result_does_not_depend_on_row_order) {
    std::mt19937 rng(99);
    const BooleanMatrix basic = toBasicMatrix(BooleanMatrix::fromGrid(randomGrid(rng, 15, 9, 0.4)));
    auto rows = basic.rowSets();
    std::reverse(rows.begin(), rows.end());
    CHECK(YycSolver::solve(basic) == YycSolver::solve(BooleanMatrix(basic.cols(), rows)));
}

TEST(yyc_trace_records_decisions) {
    const auto m = BooleanMatrix::fromGrid({{1, 1, 0}, {0, 1, 1}, {1, 0, 1}});
    YycOptions options;
    options.recordTrace = true;
    YycSolver solver(m, options);
    solver.processNextRow();
    CHECK(solver.lastTrace().decisions.empty());  // inicialización
    CHECK_EQ(solver.current().size(), 2u);        // {x1}, {x2}
    solver.processNextRow();
    // {x1} no cubre r2: se prueba con x2 (rechazado, x1 sin fila típica) y x3 (aceptado).
    // {x2} cubre r2 y se conserva.
    const auto& decisions = solver.lastTrace().decisions;
    CHECK_EQ(decisions.size(), 2u);
    CHECK(!decisions[0].coversRow);
    CHECK_EQ(decisions[0].extensions.size(), 2u);
    CHECK(!decisions[0].extensions[0].accepted);
    CHECK(decisions[0].extensions[0].columnWithoutTypicalRow == std::optional<std::size_t>(0));
    CHECK(decisions[0].extensions[1].accepted);
    CHECK(decisions[1].coversRow);
    CHECK(decisions[1].extensions.empty());
}

TEST(yyc_rejects_empty_matrix_and_extra_steps) {
    CHECK_THROWS(YycSolver(BooleanMatrix()), std::invalid_argument);
    YycSolver solver(BooleanMatrix::fromGrid({{1}}));
    solver.processNextRow();
    CHECK_THROWS(solver.processNextRow(), std::logic_error);
}

TEST(yyc_enforces_resource_limit) {
    // Filas disjuntas de 2 unos: |Ψ*| se duplica en cada fila (2^k).
    std::vector<std::vector<int>> grid;
    for (std::size_t i = 0; i < 6; ++i) {
        std::vector<int> row(12, 0);
        row[2 * i] = row[2 * i + 1] = 1;
        grid.push_back(row);
    }
    YycOptions options;
    options.maxCompatibleSets = 10;
    CHECK_THROWS(YycSolver::solve(BooleanMatrix::fromGrid(grid), options), YycResourceLimitExceeded);
    CHECK_EQ(YycSolver::solve(BooleanMatrix::fromGrid(grid)).size(), 64u);
}
