#include <charconv>
#include <chrono>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include "testores/application/experiments.hpp"
#include "testores/infrastructure/experiment_export.hpp"
#include "testores/infrastructure/matrix_file.hpp"
#include "testores/infrastructure/system_adapters.hpp"

namespace {
std::size_t positive(const std::string& value, std::size_t maximum) {
    std::size_t number = 0;
    const auto parsed = std::from_chars(value.data(),value.data()+value.size(),number);
    if (parsed.ec != std::errc{} || parsed.ptr != value.data()+value.size() || number == 0 || number > maximum)
        throw std::invalid_argument("Entero positivo fuera de rango: " + value);
    return number;
}
class Progress final : public testores::application::ExperimentProgress {
public:
    explicit Progress(testores::infrastructure::ExperimentCsv& csv) : csv_(csv) {}
    void onMeasurement(const testores::application::ExperimentCase& experiment, std::size_t index,
                       const testores::application::ExperimentMeasurement& measurement) override {
        csv_.append(experiment,index,measurement);
        std::cout << experiment.family << '^' << experiment.copies << ' '
                  << (index < 2 ? "YYC" : "BT") << ' ' << (index % 2 == 0 ? "original" : "ascendente")
                  << ": " << measurement.status;
        if (measurement.status == "ok")
            std::cout << " " << std::fixed << std::setprecision(3)
                      << static_cast<double>(measurement.median.count())/1000000.0 << " ms";
        else std::cout << " (" << measurement.detail << ')';
        std::cout << std::endl;
    }
private:
    testores::infrastructure::ExperimentCsv& csv_;
};
}

int main(int argc, char* argv[]) {
    try {
        std::map<std::string,std::string> args;
        bool overwrite = false;
        const std::set<std::string> valid{"--entrada","--salida","--repeticiones","--limite-ms","--familia","--n"};
        for (int i=1;i<argc;++i) {
            const std::string key=argv[i];
            if (key=="--ayuda" || key=="--help") {
                std::cout << "Ejercicios 4/5: B y tablas de phi^N(theta(A,B)) y gamma^N(theta(A,B)).\n"
                          << "testores_experimentos [--entrada data/matriz_A.txt] [--salida docs/resultados]\n"
                          << "  [--repeticiones 9] [--limite-ms 10000] [--familia phi|gamma|todo] [--n 1..5] [--forzar]\n"
                          << "El límite es por ejecución; se exporta 'limite', nunca un resultado parcial.\n";
                return 0;
            }
            if (key=="--forzar") {
                if (overwrite) throw std::invalid_argument("Opción repetida: --forzar");
                overwrite=true; continue;
            }
            if (!valid.count(key) || i+1>=argc) throw std::invalid_argument("Opción no válida o sin valor: " + key);
            if (!args.emplace(key,argv[++i]).second) throw std::invalid_argument("Opción repetida: " + key);
        }
        const auto value=[&](const std::string& key,const std::string& fallback) {
            const auto found=args.find(key); return found==args.end()?fallback:found->second;
        };
        testores::application::ExperimentOptions options;
        options.repetitions=positive(value("--repeticiones","9"),10000);
        options.timeLimit=std::chrono::milliseconds(positive(value("--limite-ms","10000"),3600000));
        const std::string family=value("--familia","todo");
        if (family!="todo" && family!="phi" && family!="gamma") throw std::invalid_argument("Familia desconocida");
        const std::size_t n=args.count("--n")?positive(args.at("--n"),5):0;
        const auto a=testores::infrastructure::loadMatrix(value("--entrada","data/matriz_A.txt"));
        const auto b=testores::application::buildDualMatrix(a);
        const auto cases=testores::application::buildExperimentCases(a);
        const std::filesystem::path directory=value("--salida","docs/resultados");
        // Verificar todos los destinos antes de modificar alguno.
        std::vector<std::filesystem::path> destinations{directory/"experimentos.csv",directory/"experimentos_phi.tex",
            directory/"experimentos_gamma.tex",directory/"matriz_B.txt"};
        for (const auto& experiment : cases)
            if ((family=="todo" || experiment.family==family) && (n==0 || experiment.copies==n))
                destinations.push_back(directory/(experiment.family+"_"+std::to_string(experiment.copies)+".txt"));
        if (destinations.size()==4) throw std::invalid_argument("No hay experimentos para ese filtro");
        if (!overwrite) for (const auto& path:destinations)
            if (std::filesystem::exists(path)) throw std::runtime_error("Ya existe " + path.string() + "; use --forzar");
        testores::infrastructure::saveMatrix(directory/"matriz_B.txt",b,{"B: testores típicos de A en orden canónico"},overwrite);
        testores::infrastructure::ExperimentCsv csv(directory/"experimentos.csv",overwrite);
        Progress progress(csv);
        testores::infrastructure::SteadyClock clock;
        testores::infrastructure::ExperimentResults results;
        for (const auto& experiment:cases) {
            if ((family!="todo" && experiment.family!=family) || (n!=0 && experiment.copies!=n)) continue;
            testores::infrastructure::saveMatrix(directory/(experiment.family+"_"+std::to_string(experiment.copies)+".txt"),
                experiment.matrix,{"Matriz experimental: "+experiment.family+"^"+std::to_string(experiment.copies)+"(theta(A,B))"},overwrite);
            auto measured=testores::application::measureExperiment(experiment,clock,options,&progress);
            results.emplace_back(experiment,std::move(measured));
        }
        csv.close();
        testores::infrastructure::writeExperimentTables(directory,results,options,overwrite);
        std::cout << "Tablas, CSV y matrices guardados en " << directory.string() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n'; return 1;
    }
}
