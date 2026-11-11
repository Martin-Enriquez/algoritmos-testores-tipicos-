#include "test_framework.hpp"
#include <algorithm>
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
    CHECK_THROWS(repeatPhi(a,0), std::invalid_argument);
    CHECK_THROWS(repeatGamma(a,0), std::invalid_argument);
    CHECK_THROWS(theta(BooleanMatrix{},a), std::invalid_argument);
    CHECK_THROWS(repeatPhi(a,257), std::length_error);
    CHECK_THROWS(repeatGamma(a,257), std::length_error);
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
    CHECK_EQ(YycSolver::solve(repeatPhi(base,2)).size(),108U);
    CHECK_EQ(YycSolver::solve(repeatGamma(base,2)).size(),196U);
    const auto sorted = sortRowsAscending(base);
    CHECK(YycSolver::solve(sorted) == YycSolver::solve(base));
    for (std::size_t i=1;i<sorted.rows();++i)
        CHECK(sorted.row(i-1).count() <= sorted.row(i).count());
}
