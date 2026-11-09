#include "testores/infrastructure/result_export.hpp"

#include <cstdint>
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
    // Un entorno matemático por conjunto para que LaTeX pueda cortar la línea entre ellos.
    std::string out;
    for (std::size_t i = 0; i < sets.size(); ++i) {
        out += (i > 0 ? ", " : "") + ("$" + latexFeatureSet(sets[i]) + "$");
    }
    return out;
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
    output << "\\begin{tabular}{c c >{\\raggedright\\arraybackslash}p{9.5cm} r}\n\\toprule\n";
    output << "Fila & $|\\Psi^*|$ & Testores típicos de las filas procesadas & T. acum. ($\\mu$s) \\\\\n";
    output << "\\midrule\n";
    for (const auto& row : result.rows) {
        output << "$r_{" << (row.rowIndex + 1) << "}$ & " << row.testorCount << " & "
               << (row.testorsOmitted ? std::string("(omitidos por tamaño)") : latexFeatureSets(row.testors))
               << " & " << microseconds(row.cumulative) << " \\\\\n";
    }
    output << "\\bottomrule\n\\end{tabular}\n";
}

void writeBtCsv(std::ostream& output, const application::BtRunResult& result) {
    output << "orden,alfa,testor,tipico\n";
    for (std::size_t i = 0; i < result.run.testors.size(); ++i) {
        const auto& testor = result.run.testors[i];
        output << (i + 1) << ',' << domain::toBinaryString(testor.set) << ','
               << csvField(domain::formatFeatureSet(testor.set)) << ',' << (testor.typical ? "si" : "no") << '\n';
    }
}

void writeBtSummaryLatex(std::ostream& output, const domain::BooleanMatrix& matrix,
                         const application::BtRunResult& result) {
    const auto& stats = result.run.stats;
    output << "% Generado por testores (BT). Tiempo: mediana de " << result.repetitions
           << " ejecuciones, en microsegundos.\n";
    output << "\\begin{tabular}{l >{\\raggedright\\arraybackslash}p{9.5cm}}\n\\toprule\n";
    output << "n-uplos analizados & " << stats.candidatesEvaluated;
    if (matrix.cols() <= 63) {
        output << " de $2^{" << matrix.cols() << "}-1=" << ((std::uint64_t{1} << matrix.cols()) - 1) << "$";
    }
    output << " \\\\\n";
    output << "Testores encontrados & " << stats.testorsFound << " (" << stats.typicalFound << " típicos, "
           << (stats.testorsFound - stats.typicalFound) << " no típicos) \\\\\n";
    output << "Testores típicos & " << latexFeatureSets(result.run.typicalTestors) << " \\\\\n";
    output << "Tiempo total ($\\mu$s) & " << microseconds(result.total) << " \\\\\n";
    output << "\\bottomrule\n\\end{tabular}\n";
}

void writeBtTraceLatex(std::ostream& output, const domain::BooleanMatrix& matrix, const domain::BtResult& result) {
    output << "% Generado por testores (BT --traza). Requiere el paquete booktabs.\n";
    output << "\\begin{tabular}{r c l l l c c}\n\\toprule\n";
    output << "Paso & $\\alpha$ & $T_\\alpha$ & Decisión & Justificación & Salto & $\\alpha'$ \\\\\n\\midrule\n";
    for (std::size_t s = 0; s < result.trace.size(); ++s) {
        const domain::BtStep& step = result.trace[s];
        output << (s + 1) << " & \\texttt{" << domain::toBinaryString(step.candidate) << "} & $"
               << latexFeatureSet(step.candidate) << "$ & ";
        switch (step.verdict) {
            case domain::BtVerdict::NotTestor: {
                if (step.zeroRowIsNull) {
                    output << "no testor & $r_{" << (step.zeroRow + 1) << "}$ es nula & ---";
                    break;
                }
                // Filas sin cubrir; en negrita, la de último 1 más a la izquierda (la que define el salto).
                output << "no testor & sin cubrir: $";
                bool first = true;
                for (std::size_t i = 0; i < matrix.rows(); ++i) {
                    if (matrix.row(i).intersects(step.candidate)) {
                        continue;
                    }
                    output << (first ? "" : ",") << (i == step.zeroRow ? "\\mathbf{r}_{\\mathbf{" : "r_{") << (i + 1)
                           << (i == step.zeroRow ? "}}" : "}");
                    first = false;
                }
                output << "$ & S1: $x_{" << (step.pivot + 1) << "}$";
                break;
            }
            case domain::BtVerdict::TypicalTestor: {
                // Primera fila típica de cada rasgo (existe por ser típico).
                output << "\\textbf{típico} & $";
                bool first = true;
                for (std::size_t col : step.candidate.indices()) {
                    for (std::size_t i = 0; i < matrix.rows(); ++i) {
                        const ColumnSet hit = matrix.row(i) & step.candidate;
                        if (hit.count() == 1 && hit.test(col)) {
                            output << (first ? "" : ",\\ ") << "x_{" << (col + 1) << "}{:}r_{" << (i + 1) << "}";
                            first = false;
                            break;
                        }
                    }
                }
                output << "$ & S2: $x_{" << (step.pivot + 1) << "}$";
                break;
            }
            case domain::BtVerdict::NonTypicalTestor:
                output << "no típico & $x_{" << (step.columnWithoutTypicalRow.value_or(0) + 1)
                       << "}$ sin fila típica & S2: $x_{" << (step.pivot + 1) << "}$";
                break;
        }
        output << " & " << (step.next ? "\\texttt{" + domain::toBinaryString(*step.next) + "}" : std::string("fin"))
               << " \\\\\n";
    }
    output << "\\bottomrule\n\\end{tabular}\n";
}

void exportBtCsv(const std::filesystem::path& path, const application::BtRunResult& result, bool overwrite) {
    std::ofstream file = openOutputFile(path, overwrite);
    writeBtCsv(file, result);
    closeOutputFile(file, path);
}

void exportBtSummaryLatex(const std::filesystem::path& path, const domain::BooleanMatrix& matrix,
                          const application::BtRunResult& result, bool overwrite) {
    std::ofstream file = openOutputFile(path, overwrite);
    writeBtSummaryLatex(file, matrix, result);
    closeOutputFile(file, path);
}

void exportBtTraceLatex(const std::filesystem::path& path, const domain::BooleanMatrix& matrix,
                        const domain::BtResult& result, bool overwrite) {
    std::ofstream file = openOutputFile(path, overwrite);
    writeBtTraceLatex(file, matrix, result);
    closeOutputFile(file, path);
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
