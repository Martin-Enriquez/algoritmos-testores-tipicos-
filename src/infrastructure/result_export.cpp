#include "testores/infrastructure/result_export.hpp"

#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>

#include "testores/domain/notation.hpp"
#include "testores/infrastructure/output_file.hpp"

namespace testores::infrastructure {

using application::YycRunResult;
using domain::ColumnSet;

namespace {

std::string microseconds(std::chrono::nanoseconds ns) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(3) << static_cast<double>(ns.count()) / 1000.0;
    return out.str();
}

// RFC 4180: comillas dobles alrededor y comillas internas duplicadas.
std::string csvField(const std::string& text) {
    std::string out = "\"";
    for (char c : text) {
        out += (c == '"') ? std::string("\"\"") : std::string(1, c);
    }
    return out + "\"";
}

std::string latexFeatureSet(const ColumnSet& set) {
    std::string out = "\\{";
    bool first = true;
    for (std::size_t col : set.indices()) {
        out += first ? "" : ",";
        out += "x_{" + std::to_string(col + 1) + "}";
        first = false;
    }
    return out + "\\}";
}

std::string latexFeatureSets(const std::vector<ColumnSet>& sets) {
    if (sets.empty()) {
        return "$\\emptyset$";
    }
    std::string out = "$";
    for (std::size_t i = 0; i < sets.size(); ++i) {
        out += (i > 0 ? ",\\ " : "") + latexFeatureSet(sets[i]);
    }
    return out + "$";
}

}  // namespace

void writeYycCsv(std::ostream& output, const YycRunResult& result) {
    output << "fila,num_testores,testores,tiempo_acumulado_us\n";
    for (const auto& row : result.rows) {
        const std::string testors = row.testorsOmitted ? "(omitidos por tamaño)" : domain::formatFeatureSets(row.testors);
        output << (row.rowIndex + 1) << ',' << row.testorCount << ',' << csvField(testors) << ','
               << microseconds(row.cumulative) << '\n';
    }
}

void writeYycLatexTable(std::ostream& output, const YycRunResult& result) {
    output << "% Generado por testores (YYC). Tiempos: mediana de " << result.repetitions
           << " ejecuciones, en microsegundos.\n";
    output << "\\begin{tabular}{c c p{9cm} r}\n\\toprule\n";
    output << "Fila & $|\\Psi^*|$ & Testores típicos de las filas procesadas & Tiempo acum. ($\\mu$s) \\\\\n";
    output << "\\midrule\n";
    for (const auto& row : result.rows) {
        output << "$r_{" << (row.rowIndex + 1) << "}$ & " << row.testorCount << " & "
               << (row.testorsOmitted ? std::string("(omitidos por tamaño)") : latexFeatureSets(row.testors))
               << " & " << microseconds(row.cumulative) << " \\\\\n";
    }
    output << "\\bottomrule\n\\end{tabular}\n";
}

void writeMatrixLatex(std::ostream& output, const domain::BooleanMatrix& matrix) {
    output << "\\begin{pmatrix}\n";
    for (const ColumnSet& row : matrix.rowSets()) {
        for (std::size_t j = 0; j < matrix.cols(); ++j) {
            output << (j > 0 ? " & " : "") << (row.test(j) ? '1' : '0');
        }
        output << " \\\\\n";
    }
    output << "\\end{pmatrix}\n";
}

void exportYycCsv(const std::filesystem::path& path, const YycRunResult& result, bool overwrite) {
    std::ofstream file = openOutputFile(path, overwrite);
    writeYycCsv(file, result);
    closeOutputFile(file, path);
}

void exportYycLatexTable(const std::filesystem::path& path, const YycRunResult& result, bool overwrite) {
    std::ofstream file = openOutputFile(path, overwrite);
    writeYycLatexTable(file, result);
    closeOutputFile(file, path);
}

}  // namespace testores::infrastructure
