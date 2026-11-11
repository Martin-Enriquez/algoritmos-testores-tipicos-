#include <stdexcept>

#include "test_framework.hpp"
#include "testores/domain/basic_matrix.hpp"
#include "testores/domain/boolean_matrix.hpp"
#include "testores/domain/column_set.hpp"
#include "testores/domain/notation.hpp"
#include "testores/domain/testor.hpp"
#include "testores/domain/matrix_operators.hpp"
#include "testores/domain/yyc.hpp"
#include "testores/domain/bt.hpp"

using namespace testores::domain;

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

TEST(column_set_basic_operations) {
    ColumnSet a = ColumnSet::fromIndices(5, {0, 2});
    ColumnSet b = ColumnSet::fromIndices(5, {2, 4});
    CHECK_EQ(a.count(), 2u);
    CHECK(a.test(0) && !a.test(1));
    CHECK(a.intersects(b));
    CHECK((a & b) == ColumnSet::fromIndices(5, {2}));
    CHECK((a | b) == ColumnSet::fromIndices(5, {0, 2, 4}));
    CHECK(ColumnSet::fromIndices(5, {2}).isSubsetOf(a));
    CHECK(!a.isSubsetOf(b));
    CHECK(ColumnSet(5).none());
}

TEST(column_set_spans_several_words) {
    ColumnSet s(130);
    s.set(0);
    s.set(64);
    s.set(129);
    CHECK_EQ(s.count(), 3u);
    CHECK(s.indices() == (std::vector<std::size_t>{0, 64, 129}));
    s.set(64, false);
    CHECK_EQ(s.count(), 2u);
}

TEST(column_set_rejects_invalid_access) {
    ColumnSet s(4);
    CHECK_THROWS(s.set(4), std::out_of_range);
    CHECK_THROWS(s.test(10), std::out_of_range);
    CHECK_THROWS(s.intersects(ColumnSet(5)), std::invalid_argument);
    CHECK_THROWS(ColumnSet(ColumnSet::kMaxWidth + 1), std::length_error);
}

TEST(canonical_order_is_size_then_lexicographic) {
    const ColumnSet a = ColumnSet::fromIndices(4, {2});
    const ColumnSet b = ColumnSet::fromIndices(4, {0, 3});
    const ColumnSet c = ColumnSet::fromIndices(4, {1, 2});
    CHECK(canonicalLess(a, b));
    CHECK(canonicalLess(b, c));
    CHECK(!canonicalLess(c, b));
}

TEST(matrix_validation) {
    CHECK_THROWS(BooleanMatrix::fromGrid({{1, 0}, {1}}), std::invalid_argument);
    CHECK_THROWS(BooleanMatrix::fromGrid({{1, 2}}), std::invalid_argument);
    CHECK_THROWS(BooleanMatrix(3, {ColumnSet(2)}), std::invalid_argument);
    CHECK_THROWS(BooleanMatrix(MatrixLimits::kMaxCols + 1, {}), std::invalid_argument);
}

TEST(density_is_ones_over_entries) {
    const auto m = BooleanMatrix::fromGrid({{1, 0, 1, 0}, {1, 1, 1, 1}});
    CHECK_EQ(m.onesCount(), 6u);
    CHECK(m.density() == 0.75);
    CHECK(BooleanMatrix().density() == 0.0);
}

TEST(subrow_relation) {
    const ColumnSet p = ColumnSet::fromIndices(4, {0});
    const ColumnSet t = ColumnSet::fromIndices(4, {0, 2});
    CHECK(isSubrow(p, t));
    CHECK(!isSubrow(t, p));
    CHECK(!isSubrow(t, t));  // una fila no es subfila de sí misma
}

TEST(basic_reduction_removes_zero_duplicate_and_superrows) {
    const auto m = BooleanMatrix::fromGrid({
        {1, 1, 0, 0},  // r1: superfila de r3
        {0, 0, 0, 0},  // r2: nula
        {1, 0, 0, 0},  // r3
        {0, 1, 1, 0},  // r4
        {1, 0, 0, 0},  // r5: repetida de r3
        {0, 1, 1, 1},  // r6: superfila de r4
        {0, 0, 1, 1},  // r7
    });
    const BasicReduction red = reduceToBasic(m);
    CHECK(red.basic == BooleanMatrix::fromGrid({{1, 0, 0, 0}, {0, 1, 1, 0}, {0, 0, 1, 1}}));
    CHECK(red.decisions[0].fate == RowFate::Superrow && red.decisions[0].witness == 2);
    CHECK(red.decisions[1].fate == RowFate::ZeroRow);
    CHECK(red.decisions[4].fate == RowFate::Duplicate && red.decisions[4].witness == 2);
    CHECK(red.decisions[5].fate == RowFate::Superrow && red.decisions[5].witness == 3);
    CHECK(red.decisions[6].fate == RowFate::Kept);
    CHECK(isBasicMatrix(red.basic));
    CHECK(!isBasicMatrix(m));
    CHECK_EQ(red.basic.cols(), 4u);
}

TEST(superrow_witness_is_a_kept_row) {
    // r3 ⊃ r2 ⊃ r1: el testigo de r3 debe ser r1 (que se conserva), no r2.
    const auto m = BooleanMatrix::fromGrid({{1, 0, 0}, {1, 1, 0}, {1, 1, 1}});
    const BasicReduction red = reduceToBasic(m);
    CHECK(red.decisions[1].fate == RowFate::Superrow && red.decisions[1].witness == 0);
    CHECK(red.decisions[2].fate == RowFate::Superrow && red.decisions[2].witness == 0);
}

TEST(basic_reduction_is_idempotent) {
    const auto m = BooleanMatrix::fromGrid({{1, 1, 0}, {1, 0, 0}, {0, 1, 1}, {0, 1, 1}});
    const BooleanMatrix once = toBasicMatrix(m);
    CHECK(toBasicMatrix(once) == once);
}

TEST(all_zero_matrix_reduces_to_empty) {
    const auto m = BooleanMatrix::fromGrid({{0, 0}, {0, 0}});
    CHECK_EQ(toBasicMatrix(m).rows(), 0u);
}

TEST(testor_and_typical_testor_definitions) {
    const auto m = BooleanMatrix::fromGrid({{1, 0, 0, 1}, {0, 1, 0, 1}, {0, 0, 1, 1}});
    CHECK(isTestor(m, ColumnSet::fromIndices(4, {3})));
    CHECK(isTypicalTestor(m, ColumnSet::fromIndices(4, {3})));
    CHECK(isTypicalTestor(m, ColumnSet::fromIndices(4, {0, 1, 2})));
    CHECK(isTestor(m, ColumnSet::fromIndices(4, {0, 3})));
    CHECK(!isTypicalTestor(m, ColumnSet::fromIndices(4, {0, 3})));  // {x4} ya es testor
    CHECK(!isTestor(m, ColumnSet::fromIndices(4, {0, 1})));
    CHECK(!isTypicalTestor(m, ColumnSet(4)));
    // En {x1, x4}, x1 sólo aparece en r1 junto con x4: no tiene fila típica.
    CHECK(columnWithoutTypicalRow(m, ColumnSet::fromIndices(4, {0, 3}), 3) == std::optional<std::size_t>(0));
}

TEST(notation) {
    const ColumnSet s = ColumnSet::fromIndices(5, {0, 3});
    CHECK_EQ(featureName(0), std::string("x1"));
    CHECK_EQ(formatFeatureSet(s), std::string("{x1, x4}"));
    CHECK_EQ(toBinaryString(s), std::string("10010"));
    CHECK_EQ(formatFeatureSets({}), std::string("∅"));
}
