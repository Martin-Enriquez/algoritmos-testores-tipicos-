#pragma once

#include "testores/application/io_ports.hpp"

namespace testores::infrastructure {

class FileMatrixRepository final : public application::MatrixRepository {
public:
    domain::BooleanMatrix load(const std::string& path) override;
    domain::BooleanMatrix parse(std::istream& input) override;
    bool exists(const std::string& path) override;
    void save(const std::string& path, const domain::BooleanMatrix& matrix,
              const std::vector<std::string>& comments, bool overwrite) override;
};

class FileResultExporter final : public application::ResultExporter {
public:
    void yycCsv(const std::string& path, const application::YycRunResult& result, bool overwrite) override;
    void yycLatex(const std::string& path, const application::YycRunResult& result, bool overwrite) override;
    void btCsv(const std::string& path, const application::BtRunResult& result, bool overwrite) override;
    void btLatex(const std::string& path, const domain::BooleanMatrix& matrix,
                 const application::BtRunResult& result, bool overwrite) override;
    void btTraceLatex(const std::string& path, const domain::BooleanMatrix& matrix,
                      const domain::BtResult& result, bool overwrite) override;
    void matrixLatex(const std::string& path, const domain::BooleanMatrix& matrix, bool overwrite) override;
};

class MersenneRandomFactory final : public application::RandomSourceFactory {
public:
    std::uint64_t freshSeed() override;
    std::unique_ptr<application::RandomSource> create(std::uint64_t seed) override;
};

}  // namespace testores::infrastructure
