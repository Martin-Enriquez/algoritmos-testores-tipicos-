#include "testores/domain/yyc.hpp"

#include <algorithm>
#include <string>
#include <utility>

#include "testores/domain/testor.hpp"

namespace testores::domain {

YycSolver::YycSolver(BooleanMatrix matrix, YycOptions options)
    : matrix_(std::move(matrix)), options_(options) {
    if (matrix_.empty()) {
        throw std::invalid_argument("YYC requiere una matriz con al menos una fila y una columna");
    }
}

void YycSolver::enforceLimit(std::size_t size) const {
    if (size > options_.maxCompatibleSets) {
        throw YycResourceLimitExceeded("YYC superó el límite de " + std::to_string(options_.maxCompatibleSets) +
                                       " conjuntos compatibles");
    }
}

void YycSolver::initializeWithFirstRow(YycStepStats& stats) {
    const ColumnSet& first = matrix_.row(0);
    for (std::size_t col : first.indices()) {
        ColumnSet singleton(matrix_.cols());
        singleton.set(col);
        current_.push_back(std::move(singleton));
    }
    stats.candidatesTried = current_.size();
    stats.candidatesAccepted = current_.size();
}

YycStepStats YycSolver::processNextRow() {
    if (finished()) {
        throw std::logic_error("YYC: no quedan filas por procesar");
    }
    const std::size_t k = processed_;
    YycStepStats stats;
    stats.rowIndex = k;
    lastTrace_ = YycStepTrace{};
    lastTrace_.rowIndex = k;

    if (k == 0) {
        initializeWithFirstRow(stats);
    } else {
        const ColumnSet& row = matrix_.row(k);
        const std::vector<std::size_t> rowColumns = row.indices();
        std::vector<ColumnSet> next;
        next.reserve(current_.size());

        for (ColumnSet& tau : current_) {
            const bool covers = tau.intersects(row);
            YycTauDecision* decision = nullptr;
            if (options_.recordTrace) {
                lastTrace_.decisions.push_back({tau, covers, {}});
                decision = &lastTrace_.decisions.back();
            }
            if (covers) {
                next.push_back(std::move(tau));
                ++stats.kept;
                enforceLimit(next.size());
                continue;
            }
            for (std::size_t x : rowColumns) {
                ColumnSet candidate = tau;
                candidate.set(x);
                ++stats.candidatesTried;
                const bool accepted = isCompatibleSet(matrix_, candidate, k + 1);
                if (decision != nullptr) {
                    std::optional<std::size_t> blocking;
                    if (!accepted) {
                        blocking = columnWithoutTypicalRow(matrix_, candidate, k + 1);
                    }
                    decision->extensions.push_back({x, accepted, blocking});
                }
                if (accepted) {
                    next.push_back(std::move(candidate));
                    ++stats.candidatesAccepted;
                    enforceLimit(next.size());
                }
            }
        }
        current_ = std::move(next);
    }

    ++processed_;
    stats.resultSize = current_.size();
    return stats;
}

std::vector<ColumnSet> YycSolver::solve(const BooleanMatrix& matrix, YycOptions options) {
    YycSolver solver(matrix, options);
    while (!solver.finished()) {
        solver.processNextRow();
    }
    std::vector<ColumnSet> result = solver.current();
    std::sort(result.begin(), result.end(), canonicalLess);
    return result;
}

}  // namespace testores::domain
