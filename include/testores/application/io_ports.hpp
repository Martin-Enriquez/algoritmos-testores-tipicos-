#pragma once

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

#include "testores/application/bt_runner.hpp"
#include "testores/application/yyc_runner.hpp"

namespace testores::application {

// La presentación conoce contratos, no el formato físico ni el sistema de archivos.
class MatrixRepository {
public:
    virtual ~MatrixRepository() = default;
    virtual domain::BooleanMatrix load(const std::string& path) = 0;
    virtual domain::BooleanMatrix parse(std::istream& input) = 0;
    virtual bool exists(const std::string& path) = 0;
    virtual void save(const std::string& path, const domain::BooleanMatrix& matrix,
                      const std::vector<std::string>& comments, bool overwrite) = 0;
};

class ResultExporter {
public:
    virtual ~ResultExporter() = default;
    virtual void yycCsv(const std::string& path, const YycRunResult& result, bool overwrite) = 0;
    virtual void yycLatex(const std::string& path, const YycRunResult& result, bool overwrite) = 0;
    virtual void btCsv(const std::string& path, const BtRunResult& result, bool overwrite) = 0;
    virtual void btLatex(const std::string& path, const domain::BooleanMatrix& matrix,
                         const BtRunResult& result, bool overwrite) = 0;
    virtual void btTraceLatex(const std::string& path, const domain::BooleanMatrix& matrix,
                              const domain::BtResult& result, bool overwrite) = 0;
    virtual void matrixLatex(const std::string& path, const domain::BooleanMatrix& matrix,
                             bool overwrite) = 0;
};

class RandomSourceFactory {
public:
    virtual ~RandomSourceFactory() = default;
    virtual std::uint64_t freshSeed() = 0;
    virtual std::unique_ptr<RandomSource> create(std::uint64_t seed) = 0;
};

}  // namespace testores::application
