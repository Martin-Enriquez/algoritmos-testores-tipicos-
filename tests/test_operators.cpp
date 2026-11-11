#include <algorithm>
#include <stdexcept>
#include <vector>

#include "test_framework.hpp"
#include "testores/domain/matrix_operators.hpp"
#include "testores/domain/basic_matrix.hpp"
#include "testores/domain/bt.hpp"
#include "testores/domain/yyc.hpp"

using namespace testores::domain;

TEST(operators_have_the_paper_block_structure) {
    const auto a = BooleanMatrix::fromGrid({{1,0},{0,1}});
    const auto b = BooleanMatrix::fromGrid({{1,1}});
    CHECK(theta(a,b) == BooleanMatrix::fromGrid({{1,0,1,1},{0,1,1,1}}));
    CHECK(gamma(a,b) == BooleanMatrix::fromGrid({{1,0,0,0},{0,1,0,0},{0,0,1,1}}));
    CHECK(phi(a,a) == BooleanMatrix::fromGrid({{1,0,1,0},{0,1,0,1}}));
    CHECK_THROWS(phi(a,b), std::invalid_argument);
    CHECK_THROWS(phiPower(a,0), std::invalid_argument);
    CHECK_THROWS(gammaPower(a,0), std::invalid_argument);
    CHECK_THROWS(theta(BooleanMatrix{},a), std::invalid_argument);
    CHECK_THROWS(phiPower(a,257), std::invalid_argument);
    CHECK_THROWS(gammaPower(a,257), std::invalid_argument);
}

TEST(operators_typical_testor_identities_and_duality) {
    const auto a = BooleanMatrix::fromGrid({{1,0,1,0,1,0,1},{1,0,0,0,1,1,0},
        {1,1,0,1,0,0,0},{0,0,1,1,0,1,1},{1,0,0,1,0,1,0},{1,0,0,1,0,0,1}});
    const auto ta = YycSolver::solve(a);
    const BooleanMatrix b(a.cols(),ta);
    CHECK(isBasicMatrix(b));
    auto expected = a.rowSets();
    std::sort(expected.begin(),expected.end(),canonicalLess);
    CHECK(BtSolver::solve(b).typicalTestors == expected);
    const auto base = theta(a,b);
    CHECK_EQ(base.rows(),48U);
    CHECK_EQ(base.cols(),14U);
    CHECK(isBasicMatrix(base));
    CHECK_EQ(YycSolver::solve(base).size(),14U);
    CHECK_EQ(YycSolver::solve(phiPower(base,2)).size(),108U);
    CHECK_EQ(YycSolver::solve(gammaPower(base,2)).size(),196U);
    const auto sorted = sortRowsByOnes(base);
    CHECK(YycSolver::solve(sorted) == YycSolver::solve(base));
    for (std::size_t i=1;i<sorted.rows();++i)
        CHECK(sorted.row(i-1).count() <= sorted.row(i).count());
}

TEST(theta_preserves_all_row_pairs_and_column_positions) {
    const auto a = BooleanMatrix::fromGrid({{1, 0}, {0, 1}});
    const auto b = BooleanMatrix::fromGrid({{1, 0, 1}, {0, 1, 0}});
    const auto result = theta(a, b);
    CHECK(result == BooleanMatrix::fromGrid({{1, 0, 1, 0, 1}, {1, 0, 0, 1, 0},
                                           {0, 1, 1, 0, 1}, {0, 1, 0, 1, 0}}));
    CHECK(isBasicMatrix(result));
    // Cada bloque aporta sus propios testores típicos, sin mezclarlos.
    const std::vector<ColumnSet> expected = {
        ColumnSet::fromIndices(5, {0, 1}), ColumnSet::fromIndices(5, {2, 3}),
        ColumnSet::fromIndices(5, {3, 4})};
    CHECK(YycSolver::solve(result) == expected);
    CHECK(BtSolver::solve(result).typicalTestors == expected);
}

TEST(theta_rejects_empty_inputs_and_excessive_dimensions) {
    const auto one = BooleanMatrix::fromGrid({{1}});
    CHECK_THROWS(theta(BooleanMatrix(), one), std::invalid_argument);
    CHECK_THROWS(theta(one, BooleanMatrix()), std::invalid_argument);
    const BooleanMatrix wide(512, {ColumnSet::fromIndices(512, {511})});
    CHECK_THROWS(theta(wide, one), std::invalid_argument);
    const BooleanMatrix tall(1, std::vector<ColumnSet>(317, ColumnSet::fromIndices(1, {0})));
    CHECK_THROWS(theta(tall, tall), std::invalid_argument);
}

TEST(theta_concatenates_across_word_boundaries) {
    const BooleanMatrix a(65, {ColumnSet::fromIndices(65, {0, 64})});
    const BooleanMatrix b(66, {ColumnSet::fromIndices(66, {0, 65})});
    const auto result = theta(a, b);
    CHECK_EQ(result.cols(), 131u);
    CHECK(result.row(0) == ColumnSet::fromIndices(131, {0, 64, 65, 130}));
}

TEST(phi_joins_corresponding_rows_without_cartesian_product) {
    const auto a = BooleanMatrix::fromGrid({{1, 0}, {0, 1}});
    const auto b = BooleanMatrix::fromGrid({{1, 0, 1}, {0, 1, 0}});
    const auto result = phi(a, b);
    CHECK(result == BooleanMatrix::fromGrid({{1, 0, 1, 0, 1}, {0, 1, 0, 1, 0}}));
    CHECK(isBasicMatrix(result));
    CHECK(phiPower(a, 1) == a);
    CHECK(phiPower(a, 2) == phi(a, a));
    const std::vector<ColumnSet> expected = {
        ColumnSet::fromIndices(4, {0, 1}), ColumnSet::fromIndices(4, {0, 3}),
        ColumnSet::fromIndices(4, {1, 2}), ColumnSet::fromIndices(4, {2, 3})};
    CHECK(YycSolver::solve(phiPower(a, 2)) == expected);
    CHECK(BtSolver::solve(phiPower(a, 2)).typicalTestors == expected);
}

TEST(phi_rejects_incompatible_dimensions_and_invalid_copies) {
    const auto one = BooleanMatrix::fromGrid({{1}});
    CHECK_THROWS(phi(one, BooleanMatrix::fromGrid({{1}, {1}})), std::invalid_argument);
    CHECK_THROWS(phi(BooleanMatrix(), one), std::invalid_argument);
    CHECK_THROWS(phiPower(BooleanMatrix(), 1), std::invalid_argument);
    CHECK_THROWS(phiPower(one, 0), std::invalid_argument);
    CHECK_THROWS(phiPower(one, 513), std::invalid_argument);
    const BooleanMatrix wide(512, {ColumnSet::fromIndices(512, {511})});
    CHECK_THROWS(phi(wide, one), std::invalid_argument);
}

TEST(phi_copies_preserve_bits_across_word_boundaries) {
    const BooleanMatrix a(65, {ColumnSet::fromIndices(65, {0, 64})});
    const auto result = phiPower(a, 3);
    CHECK_EQ(result.rows(), 1u);
    CHECK_EQ(result.cols(), 195u);
    CHECK(result.row(0) == ColumnSet::fromIndices(195, {0, 64, 65, 129, 130, 194}));
    CHECK(result == phi(phi(a, a), a));
}

TEST(gamma_builds_diagonal_blocks_and_combines_typical_testors) {
    const auto a = BooleanMatrix::fromGrid({{1, 1}});
    const auto b = BooleanMatrix::fromGrid({{1, 0}, {0, 1}});
    const auto result = gamma(a, b);
    CHECK(result == BooleanMatrix::fromGrid({{1, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}));
    CHECK(isBasicMatrix(result));
    const std::vector<ColumnSet> expected = {ColumnSet::fromIndices(4, {0, 2, 3}),
                                            ColumnSet::fromIndices(4, {1, 2, 3})};
    CHECK(YycSolver::solve(result) == expected);
    CHECK(BtSolver::solve(result).typicalTestors == expected);
    CHECK(gammaPower(a, 1) == a);
    CHECK(gammaPower(a, 2) == gamma(a, a));
    CHECK_EQ(YycSolver::solve(gammaPower(a, 3)).size(), 8u);
}

TEST(gamma_rejects_empty_inputs_and_excessive_dimensions) {
    const auto one = BooleanMatrix::fromGrid({{1}});
    CHECK_THROWS(gamma(BooleanMatrix(), one), std::invalid_argument);
    CHECK_THROWS(gamma(one, BooleanMatrix()), std::invalid_argument);
    CHECK_THROWS(gammaPower(BooleanMatrix(), 1), std::invalid_argument);
    CHECK_THROWS(gammaPower(one, 0), std::invalid_argument);
    CHECK_THROWS(gammaPower(one, 513), std::invalid_argument);
    const BooleanMatrix wide(512, {ColumnSet::fromIndices(512, {511})});
    CHECK_THROWS(gamma(wide, one), std::invalid_argument);
    const BooleanMatrix tall(1, std::vector<ColumnSet>(50001, ColumnSet::fromIndices(1, {0})));
    CHECK_THROWS(gamma(tall, tall), std::invalid_argument);
    CHECK_THROWS(gammaPower(tall, 2), std::invalid_argument);
}

TEST(gamma_offsets_blocks_across_word_boundaries) {
    const BooleanMatrix a(65, {ColumnSet::fromIndices(65, {0, 64})});
    const auto result = gammaPower(a, 3);
    CHECK_EQ(result.rows(), 3u);
    CHECK_EQ(result.cols(), 195u);
    CHECK(result.row(0) == ColumnSet::fromIndices(195, {0, 64}));
    CHECK(result.row(1) == ColumnSet::fromIndices(195, {65, 129}));
    CHECK(result.row(2) == ColumnSet::fromIndices(195, {130, 194}));
    CHECK(result == gamma(gamma(a, a), a));
}

TEST(row_sort_is_ascending_stable_and_preserves_typical_testors) {
    const auto matrix = BooleanMatrix::fromGrid({{1, 1, 1, 0}, {0, 0, 0, 1}, {1, 0, 0, 0}, {0, 1, 1, 0}});
    const auto sorted = sortRowsByOnes(matrix);
    CHECK(sorted == BooleanMatrix::fromGrid({{0, 0, 0, 1}, {1, 0, 0, 0}, {0, 1, 1, 0}, {1, 1, 1, 0}}));
    CHECK(YycSolver::solve(matrix) == YycSolver::solve(sorted));
    CHECK(sortRowsByOnes(sorted) == sorted);
    CHECK(matrix == BooleanMatrix::fromGrid({{1, 1, 1, 0}, {0, 0, 0, 1}, {1, 0, 0, 0}, {0, 1, 1, 0}}));
}

TEST(row_sort_keeps_zero_rows_duplicates_and_wide_columns) {
    const auto matrix = BooleanMatrix::fromGrid({{1, 1}, {0, 0}, {1, 1}});
    CHECK(sortRowsByOnes(matrix) == BooleanMatrix::fromGrid({{0, 0}, {1, 1}, {1, 1}}));
    CHECK(sortRowsByOnes(BooleanMatrix()) == BooleanMatrix());
    const BooleanMatrix wide(130, {ColumnSet::fromIndices(130, {0, 64, 129}), ColumnSet::fromIndices(130, {129})});
    const auto sorted = sortRowsByOnes(wide);
    CHECK(sorted.row(0) == wide.row(1));
    CHECK(sorted.row(1) == wide.row(0));
}
