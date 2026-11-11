#pragma once

#include <filesystem>
#include <fstream>
#include <utility>
#include <vector>
#include "testores/application/experiments.hpp"

namespace testores::infrastructure {
using ExperimentResults = std::vector<std::pair<application::ExperimentCase, application::ExperimentMeasurements>>;

class ExperimentCsv {
public:
    ExperimentCsv(const std::filesystem::path& path, bool overwrite);
    void append(const application::ExperimentCase& experiment, std::size_t index,
                const application::ExperimentMeasurement& measurement);
    void close();
private:
    std::filesystem::path path_;
    std::ofstream stream_;
};

void writeExperimentTables(const std::filesystem::path& directory, const ExperimentResults& results,
                           const application::ExperimentOptions& options, bool overwrite);
}  // namespace testores::infrastructure
