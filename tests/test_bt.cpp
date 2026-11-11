#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
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
#include "testores/infrastructure/cli_adapters.hpp"
#include "testores/infrastructure/system_adapters.hpp"
#include "testores/presentation/cli.hpp"
#include "testores/presentation/console_view.hpp"

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

int runTestApplication(const std::vector<std::string>& args, std::istream& in,
                       std::ostream& out, std::ostream& err) {
    infrastructure::FileMatrixRepository matrices;
    infrastructure::FileResultExporter exports;
    infrastructure::MersenneRandomFactory random;
    infrastructure::SteadyClock clock;
    presentation::CliServices services{matrices, exports, random, clock};
    return presentation::runApplication(args, in, out, err, services);
}

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

TEST(bt_chooses_the_zero_row_whose_last_one_is_leftmost) {
    // "Algo BT", Paso 3: si varias filas provocan que α no sea testor, se toma
    // la que tenga el último 1 más a la izquierda. En A, α = 0000001 = {x7} no
    // cubre r2 = 1000110, r3 = 1101000 ni r5 = 1001010; la de último 1 más a la
    // izquierda es r3 (x4), así que BT salta de 0000001 a 0001000.
    BtOptions options;
    options.recordTrace = true;
    const auto result = BtSolver::solve(matrixA(), options);
    CHECK(result.trace[0].candidate == bits("0000001"));
    CHECK_EQ(result.trace[0].zeroRow, 2u);  // r3
    CHECK_EQ(result.trace[0].pivot, 3u);    // x4
    CHECK(*result.trace[0].next == bits("0001000"));
    CHECK(btZeroRow(matrixA(), bits("0000001")) == std::optional<std::size_t>(2));
    // Empate en la posición del último 1: se elige la fila de menor índice.
    const auto tie = BooleanMatrix::fromGrid({{0, 1, 0}, {1, 1, 0}, {1, 0, 1}});
    CHECK(btZeroRow(tie, bits("001")) == std::optional<std::size_t>(0));
    // Con esta regla BT analiza 44 n-uplos de A (24 testores y 20 que no lo son),
    // en lugar de los 51 que analizaría tomando siempre la primera fila sin cubrir.
    CHECK_EQ(result.stats.candidatesEvaluated, 44u);
    CHECK_EQ(result.stats.testorsFound, 24u);
    CHECK_EQ(result.stats.typicalFound, 8u);
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

TEST(both_solvers_and_basic_reduction_match_all_small_boolean_matrices) {
    // Enumeración completa, sin suponer que las entradas ya son básicas.
    // Incluye todas las matrices m x n con 1 <= m <= 4, 1 <= n <= 4
    // y m*n <= 12: filas nulas, duplicadas, superfilas y columnas nulas.
    std::size_t checked = 0;
    for (std::size_t rows = 1; rows <= 4; ++rows) {
        for (std::size_t cols = 1; cols <= 4; ++cols) {
            const std::size_t cells = rows * cols;
            if (cells > 12) continue;
            for (unsigned encoding = 0; encoding < (1U << cells); ++encoding) {
                std::vector<std::vector<int>> grid(rows, std::vector<int>(cols));
                for (std::size_t i = 0; i < rows; ++i) {
                    for (std::size_t j = 0; j < cols; ++j) {
                        grid[i][j] = static_cast<int>((encoding >> (i * cols + j)) & 1U);
                    }
                }
                const auto matrix = BooleanMatrix::fromGrid(grid);
                const auto basic = toBasicMatrix(matrix);
                const auto expected = bruteForceTypicalTestors(grid);
                CHECK(BtSolver::solve(matrix).typicalTestors == expected);
                CHECK(YycSolver::solve(matrix) == expected);
                CHECK(BtSolver::solve(basic).typicalTestors == expected);
                CHECK(YycSolver::solve(basic) == expected);
                CHECK(isBasicMatrix(basic));
                CHECK(toBasicMatrix(basic) == basic);
                // La reducción debe preservar todos los testores, no sólo
                // sus elementos minimales.
                for (unsigned mask = 0; mask < (1U << cols); ++mask) {
                    CHECK_EQ(isTestorMask(grid, mask), isTestorMask(basic.toGrid(), mask));
                }
                ++checked;
            }
        }
    }
    CHECK_EQ(checked, 9418u);
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
            if (!testor) {
                // La fila elegida no está cubierta y su último 1 es el que está más
                // a la izquierda entre las filas no cubiertas (empate: menor índice).
                const ColumnSet& chosen = basic.row(step.zeroRow);
                CHECK(!chosen.intersects(step.candidate));
                CHECK_EQ(step.zeroRowIsNull, chosen.none());
                for (std::size_t i = 0; i < basic.rows(); ++i) {
                    const ColumnSet& other = basic.row(i);
                    if (other.intersects(step.candidate) || chosen.none()) continue;
                    CHECK(!other.none());
                    CHECK(chosen.lastIndex() < other.lastIndex() ||
                          (chosen.lastIndex() == other.lastIndex() && step.zeroRow <= i));
                }
            }
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

TEST(bt_visited_tuples_do_not_depend_on_row_order) {
    // La fila del salto S1 se elige por la posición de su último 1, no por su
    // posición en la matriz: reordenar las filas no cambia el recorrido.
    auto visitedTuples = [](const BooleanMatrix& m) {
        BtOptions options;
        options.recordTrace = true;
        std::vector<ColumnSet> visited;
        for (const auto& step : BtSolver::solve(m, options).trace) visited.push_back(step.candidate);
        return visited;
    };
    const BooleanMatrix a = matrixA();
    auto rows = a.rowSets();
    std::sort(rows.begin(), rows.end(), [](const ColumnSet& x, const ColumnSet& y) { return x.count() < y.count(); });
    const BooleanMatrix sorted(a.cols(), rows);
    CHECK(BtSolver::solve(a).typicalTestors == BtSolver::solve(sorted).typicalTestors);
    CHECK(visitedTuples(a) == visitedTuples(sorted));

    std::mt19937 rng(777);
    for (int trial = 0; trial < 100; ++trial) {
        const std::size_t cols = 2 + static_cast<std::size_t>(trial % 9);
        const BooleanMatrix basic = toBasicMatrix(BooleanMatrix::fromGrid(randomGrid(rng, 12, cols, 0.35)));
        auto shuffled = basic.rowSets();
        std::shuffle(shuffled.begin(), shuffled.end(), rng);
        CHECK(visitedTuples(basic) == visitedTuples(BooleanMatrix(basic.cols(), shuffled)));
    }
}

TEST(bt_rejects_empty_matrix_and_finds_no_testor_with_a_null_row) {
    CHECK_THROWS(BtSolver::solve(BooleanMatrix()), std::invalid_argument);
    // Con una fila nula ningún subconjunto es testor: BT analiza 0...01, ve la
    // fila nula y termina. Lo mismo ocurre con YYC y con la matriz básica.
    const auto m = BooleanMatrix::fromGrid({{1, 1, 0}, {0, 0, 0}, {0, 1, 1}});
    BtOptions options;
    options.recordTrace = true;
    const auto result = BtSolver::solve(m, options);
    CHECK(result.typicalTestors.empty());
    CHECK(result.testors.empty());
    CHECK_EQ(result.stats.candidatesEvaluated, 1u);
    CHECK_EQ(result.trace.size(), 1u);
    CHECK(result.trace[0].verdict == BtVerdict::NotTestor);
    CHECK(result.trace[0].zeroRowIsNull);
    CHECK_EQ(result.trace[0].zeroRow, 1u);
    CHECK(!result.trace[0].next.has_value());
    CHECK(bruteForceTypicalTestors(m.toGrid()).empty());
    CHECK(YycSolver::solve(m).empty());
    const BooleanMatrix basic = toBasicMatrix(m);
    CHECK(BtSolver::solve(basic).typicalTestors.empty());
    CHECK(YycSolver::solve(basic).empty());

    std::ostringstream text;
    presentation::printBtTrace(text, m, result);
    CHECK(text.str().find("fila nula") != std::string::npos);
    std::ostringstream latex;
    infrastructure::writeBtTraceLatex(latex, m, result);
    CHECK(latex.str().find("es nula") != std::string::npos);
}

TEST(cli_reports_no_testors_for_a_matrix_with_a_null_row) {
    // Antes se descartaba la fila nula y se "encontraban" {x2} y {x1, x3}.
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "testores_fila_nula_prueba.txt";
    {
        std::ofstream file(path);
        file << "1 1 0\n0 0 0\n0 1 1\n";
    }
    for (const char* algorithm : {"yyc", "bt"}) {
        std::istringstream in("");
        std::ostringstream out;
        std::ostringstream err;
        const int code = runTestApplication({algorithm, "--entrada", path.string()}, in, out, err);
        CHECK_EQ(code, 0);
        CHECK(out.str().find("Testores típicos de la matriz (0)") != std::string::npos);
        CHECK(out.str().find("{x1, x3}") == std::string::npos);
    }
    std::filesystem::remove(path);
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

TEST(solvers_and_jumps_work_at_the_maximum_column_width) {
    const std::size_t width = ColumnSet::kMaxWidth;
    ColumnSet first(width);
    first.set(0);
    ColumnSet last(width);
    last.set(width - 1);
    std::vector<ColumnSet> rows;
    ColumnSet all(width);
    for (std::size_t i = 0; i < width; ++i) {
        rows.push_back(ColumnSet::fromIndices(width, {i}));
        all.set(i);
    }
    const BooleanMatrix matrix(width, rows);
    CHECK(BtSolver::solve(matrix).typicalTestors == std::vector<ColumnSet>{all});
    CHECK(YycSolver::solve(matrix) == std::vector<ColumnSet>{all});
    CHECK(btJumpNotTestor(last, 0) == first);
    CHECK(!btJumpTestor(first).has_value());
    CHECK(!btNumber(all).has_value());
}

TEST(bt_candidate_and_cooperative_limits_stop_without_returning_partial_results) {
    const auto small = BooleanMatrix::fromGrid({{1, 1, 0}, {0, 1, 1}, {1, 0, 1}});
    BtOptions options;
    options.maxCandidatesEvaluated = 5;
    CHECK_THROWS(BtSolver::solve(small, options), BtResourceLimitExceeded);
    options.maxCandidatesEvaluated = 6;
    CHECK_EQ(BtSolver::solve(small, options).stats.candidatesEvaluated, 6u);

    std::vector<ColumnSet> rows;
    for (std::size_t i = 0; i < 12; ++i) {
        rows.push_back(ColumnSet::fromIndices(24, {2 * i, 2 * i + 1}));
    }
    options = BtOptions{};
    std::size_t polls = 0;
    options.shouldStop = [&]() { return ++polls == 2; };
    CHECK_THROWS(BtSolver::solve(BooleanMatrix(24, rows), options), BtResourceLimitExceeded);
    CHECK_EQ(polls, 2u);  // inicio y después de 1024 candidatos
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
    CHECK_EQ(runTestApplication({}, in, out, err), 0);
    CHECK(out.str().find("Testores típicos de la matriz") != std::string::npos);
    CHECK(out.str().find("Tiempo total de ejecución") != std::string::npos);
    CHECK(out.str().find("Ambos procedimientos coinciden") != std::string::npos);
    CHECK(out.str().find("Paso 1: α = 00001") != std::string::npos);
}
