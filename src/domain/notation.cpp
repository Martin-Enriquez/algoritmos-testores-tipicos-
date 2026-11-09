#include "testores/domain/notation.hpp"

namespace testores::domain {

std::string featureName(std::size_t col) { return "x" + std::to_string(col + 1); }

std::string formatFeatureSet(const ColumnSet& set) {
    std::string out = "{";
    bool first = true;
    for (std::size_t col : set.indices()) {
        if (!first) {
            out += ", ";
        }
        out += featureName(col);
        first = false;
    }
    return out + "}";
}

std::string formatFeatureSets(const std::vector<ColumnSet>& sets) {
    if (sets.empty()) {
        return "∅";
    }
    std::string out;
    for (std::size_t i = 0; i < sets.size(); ++i) {
        if (i > 0) {
            out += ", ";
        }
        out += formatFeatureSet(sets[i]);
    }
    return out;
}

std::string toBinaryString(const ColumnSet& set) {
    std::string out;
    out.reserve(set.width());
    for (std::size_t col = 0; col < set.width(); ++col) {
        out += set.test(col) ? '1' : '0';
    }
    return out;
}

}  // namespace testores::domain
