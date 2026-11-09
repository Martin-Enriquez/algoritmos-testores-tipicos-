#include "testores/infrastructure/experiment_export.hpp"

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include "testores/infrastructure/output_file.hpp"

namespace testores::infrastructure {
namespace {
std::string csvText(const std::string& text) {
    std::string result = "\"";
    for (const char ch : text) {
        if (ch == '"') result += '"';
        result += ch;
    }
    return result + '"';
}
}
ExperimentCsv::ExperimentCsv(const std::filesystem::path& path, bool overwrite)
    : path_(path), stream_(openOutputFile(path,overwrite)) {
    stream_ << "familia,N,filas,columnas,tipicos_esperados,algoritmo,orden,estado,mediana_ns,repeticiones,candidatos_bt,detalle\n";
}
void ExperimentCsv::append(const application::ExperimentCase& experiment, std::size_t index,
                            const application::ExperimentMeasurement& measurement) {
    stream_ << experiment.family << ',' << experiment.copies << ',' << experiment.matrix.rows() << ','
            << experiment.matrix.cols() << ',' << experiment.expectedTypical << ','
            << (index < 2 ? "YYC" : "BT") << ',' << (index % 2 == 0 ? "original" : "ascendente")
            << ',' << measurement.status << ',';
    if (measurement.status == "ok") stream_ << measurement.median.count();
    stream_ << ',' << measurement.completedRuns << ',' << measurement.candidates << ',' << csvText(measurement.detail) << '\n';
    stream_.flush();
    if (!stream_) throw std::runtime_error("No se pudo guardar el resultado experimental");
}
void ExperimentCsv::close() { closeOutputFile(stream_,path_); }

void writeExperimentTables(const std::filesystem::path& directory, const ExperimentResults& results,
                           const application::ExperimentOptions& options, bool overwrite) {
    for (const std::string family : {"phi", "gamma"}) {
        const auto path = directory / ("experimentos_" + family + ".tex");
        auto file = openOutputFile(path,overwrite);
        file << "% Tiempos en ms; solve completo; calentamiento + " << options.repetitions << " repeticiones.\n"
             << "\\begin{tabular}{@{}clrrr|rrrr@{}}\n\\toprule\n"
             << "$N$ & Matriz & Filas & Cols. & $|\\Psi^*|$ & YYC orig. & YYC asc. & BT orig. & BT asc. \\\\\n\\midrule\n";
        file << std::fixed << std::setprecision(3);
        for (const auto& entry : results) {
            const auto& experiment = entry.first;
            if (experiment.family != family) continue;
            file << experiment.copies << " & $\\" << (family == "phi" ? "varphi" : family) << "^{" << experiment.copies
                 << "}(C)$ & " << experiment.matrix.rows() << " & " << experiment.matrix.cols()
                 << " & " << experiment.expectedTypical;
            for (const auto& measurement : entry.second) {
                file << " & ";
                if (measurement.status == "ok")
                    file << static_cast<double>(measurement.median.count()) / 1000000.0;
                else if (measurement.status == "limite")
                    file << "$\\ge " << static_cast<double>(options.timeLimit.count()) / 1000000.0 << "$";
                else file << "recurso";
            }
            file << " \\\\\n";
        }
        file << "\\bottomrule\n\\end{tabular}\n";
        closeOutputFile(file,path);
    }
}
}  // namespace testores::infrastructure
