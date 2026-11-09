#include "testores/infrastructure/cli_adapters.hpp"

#include <filesystem>

#include "testores/infrastructure/matrix_file.hpp"
#include "testores/infrastructure/output_file.hpp"
#include "testores/infrastructure/result_export.hpp"
#include "testores/infrastructure/system_adapters.hpp"

namespace testores::infrastructure {

domain::BooleanMatrix FileMatrixRepository::load(const std::string& path) { return loadMatrix(path); }
domain::BooleanMatrix FileMatrixRepository::parse(std::istream& input) { return parseMatrix(input); }
bool FileMatrixRepository::exists(const std::string& path) { return std::filesystem::exists(path); }
void FileMatrixRepository::save(const std::string& path, const domain::BooleanMatrix& matrix,
                                const std::vector<std::string>& comments, bool overwrite) {
    saveMatrix(path, matrix, comments, overwrite);
}

void FileResultExporter::yycCsv(const std::string& path, const application::YycRunResult& result,
                               bool overwrite) { exportYycCsv(path, result, overwrite); }
void FileResultExporter::yycLatex(const std::string& path, const application::YycRunResult& result,
                                 bool overwrite) { exportYycLatexTable(path, result, overwrite); }
void FileResultExporter::btCsv(const std::string& path, const application::BtRunResult& result,
                              bool overwrite) { exportBtCsv(path, result, overwrite); }
void FileResultExporter::btLatex(const std::string& path, const domain::BooleanMatrix& matrix,
                                const application::BtRunResult& result, bool overwrite) {
    exportBtSummaryLatex(path, matrix, result, overwrite);
}
void FileResultExporter::btTraceLatex(const std::string& path, const domain::BooleanMatrix& matrix,
                                     const domain::BtResult& result, bool overwrite) {
    exportBtTraceLatex(path, matrix, result, overwrite);
}
void FileResultExporter::matrixLatex(const std::string& path, const domain::BooleanMatrix& matrix,
                                    bool overwrite) {
    auto file = openOutputFile(path, overwrite);
    writeMatrixLatex(file, matrix);
    closeOutputFile(file, path);
}

std::uint64_t MersenneRandomFactory::freshSeed() { return MersenneRandomSource::freshSeed(); }
std::unique_ptr<application::RandomSource> MersenneRandomFactory::create(std::uint64_t seed) {
    return std::make_unique<MersenneRandomSource>(seed);
}

}  // namespace testores::infrastructure
