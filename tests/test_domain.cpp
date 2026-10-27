#include <stdexcept>

#include "test_framework.hpp"
#include "testores/domain/basic_matrix.hpp"
#include "testores/domain/boolean_matrix.hpp"
#include "testores/domain/column_set.hpp"
#include "testores/domain/notation.hpp"
#include "testores/domain/testor.hpp"

using namespace testores::domain;

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
