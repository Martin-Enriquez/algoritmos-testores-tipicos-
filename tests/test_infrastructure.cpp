#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "test_framework.hpp"
#include "testores/application/yyc_runner.hpp"
#include "testores/infrastructure/matrix_file.hpp"
#include "testores/infrastructure/result_export.hpp"
#include "testores/infrastructure/system_adapters.hpp"

using namespace testores;
using infrastructure::MatrixFormatError;

namespace {

domain::BooleanMatrix parse(const std::string& text) {
    std::istringstream in(text);
    return infrastructure::parseMatrix(in);
}

std::filesystem::path tempPath(const std::string& name) {
    return std::filesystem::temp_directory_path() / ("testores_test_" + name);
}

}  // namespace

TEST(parse_accepts_several_separators_and_comments) {
    const auto m = parse("# comentario\n1 0 1\n0,1,1  # fin de línea\n\n110\r\n0;0;1\n");
    CHECK_EQ(m.rows(), 4u);
    CHECK_EQ(m.cols(), 3u);
    CHECK(m == domain::BooleanMatrix::fromGrid({{1, 0, 1}, {0, 1, 1}, {1, 1, 0}, {0, 0, 1}}));
}

TEST(parse_accepts_utf8_bom) {
    CHECK(parse("\xEF\xBB\xBF" "1 0\n0 1\n") == domain::BooleanMatrix::fromGrid({{1, 0}, {0, 1}}));
}

TEST(parse_rejects_malformed_input) {
    CHECK_THROWS(parse("1 0 2\n"), MatrixFormatError);
    CHECK_THROWS(parse("1 0 1\n1 0\n"), MatrixFormatError);
    CHECK_THROWS(parse("# vacío\n\n"), MatrixFormatError);
    CHECK_THROWS(parse("1 0 x\n"), MatrixFormatError);
    CHECK_THROWS(parse(std::string(domain::MatrixLimits::kMaxCols + 1, '1') + "\n"), MatrixFormatError);
}

TEST(parse_reports_line_number) {
    try {
        parse("1 0\n0 1\n1 2\n");
        CHECK(false);
    } catch (const MatrixFormatError& error) {
        CHECK(std::string(error.what()).find("Línea 3") != std::string::npos);
    }
}

TEST(save_and_load_round_trip) {
    const auto path = tempPath("roundtrip.txt");
    std::filesystem::remove(path);
    const auto m = domain::BooleanMatrix::fromGrid({{1, 0, 1, 1}, {0, 1, 0, 0}});
    infrastructure::saveMatrix(path, m, {"prueba", "comentario\ncon salto"}, false);
    CHECK(infrastructure::loadMatrix(path) == m);
    std::filesystem::remove(path);
}

TEST(save_refuses_to_overwrite_without_permission) {
    const auto path = tempPath("overwrite.txt");
    const auto m = domain::BooleanMatrix::fromGrid({{1}});
    infrastructure::saveMatrix(path, m, {}, true);
    CHECK_THROWS(infrastructure::saveMatrix(path, m, {}, false), std::runtime_error);
    infrastructure::saveMatrix(path, m, {}, true);
    std::filesystem::remove(path);
}

TEST(comment_cannot_inject_rows) {
    std::ostringstream out;
    infrastructure::writeMatrix(out, domain::BooleanMatrix::fromGrid({{1, 0}}), {"x\n1 1"});
    std::istringstream in(out.str());
    CHECK_EQ(infrastructure::parseMatrix(in).rows(), 1u);
}

TEST(load_rejects_missing_file_and_directories) {
    CHECK_THROWS(infrastructure::loadMatrix(tempPath("no_existe.txt")), MatrixFormatError);
    CHECK_THROWS(infrastructure::loadMatrix(std::filesystem::temp_directory_path()), MatrixFormatError);
}

TEST(csv_and_latex_exports) {
    application::YycRunResult result;
    domain::ColumnSet t = domain::ColumnSet::fromIndices(3, {0, 2});
    application::YycRowRecord row;
    row.testorCount = 1;
    row.testors = {t};
    row.cumulative = std::chrono::nanoseconds(1500);
    result.rows.push_back(row);
    row.rowIndex = 1;
    row.testorCount = 7;
    row.testors.clear();
    row.testorsOmitted = true;
    result.rows.push_back(row);
    result.typicalTestors = {t};
    std::ostringstream csv;
    infrastructure::writeYycCsv(csv, result);
    CHECK(csv.str().find("1,1,\"{x1, x3}\",1.500") != std::string::npos);
    CHECK(csv.str().find("2,7,\"(omitidos") != std::string::npos);
    std::ostringstream tex;
    infrastructure::writeYycLatexTable(tex, result);
    CHECK(tex.str().find("$\\{x_{1},x_{3}\\}$") != std::string::npos);
}
