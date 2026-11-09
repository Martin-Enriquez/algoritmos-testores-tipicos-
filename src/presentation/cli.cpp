#include "testores/presentation/cli.hpp"

#include <istream>
#include <limits>
#include <optional>
#include <ostream>
#include <sstream>
#include <stdexcept>

#include "testores/application/bt_runner.hpp"
#include "testores/application/matrix_generation.hpp"
#include "testores/application/yyc_runner.hpp"
#include "testores/domain/basic_matrix.hpp"
#include "testores/domain/boolean_matrix.hpp"
#include "testores/presentation/console_input.hpp"
#include "testores/presentation/console_view.hpp"

namespace testores::presentation {

using application::BasicMatrixConstraints;
using application::BtRunResult;
using application::GenerationRequest;
using application::YycRunResult;
using domain::BooleanMatrix;
using domain::MatrixLimits;

namespace {

constexpr std::size_t kMaxRepetitions = 100000;
// Más filas que esto no se imprimen completas en consola.
constexpr std::size_t kMaxPrintedRows = 500;

const std::set<std::string> kFlags = {"--forzar", "--traza", "--completo"};

// Opciones permitidas por comando: una opción desconocida es un error, no se ignora.
const std::map<std::string, std::set<std::string>> kAllowedOptions = {
    {"generar", {"--filas", "--columnas", "--prob", "--semilla", "--salida", "--salida-basica", "--forzar"}},
    {"basica", {"--entrada", "--salida", "--forzar"}},
    {"buscar",
     {"--filas", "--columnas", "--prob", "--densidad-min", "--min-filas-basica", "--max-filas-basica", "--semilla",
      "--intentos", "--salida", "--forzar"}},
    {"yyc",
     {"--entrada", "--traza", "--completo", "--repeticiones", "--csv", "--latex", "--latex-matriz", "--forzar"}},
    {"bt", {"--entrada", "--traza", "--completo", "--repeticiones", "--csv", "--latex", "--latex-traza", "--forzar"}},
    {"ayuda", {}},
    {"menu", {}},
};

void printUsage(std::ostream& out) {
    out << "Uso: testores [comando] [opciones]\n\n"
           "Sin comando se abre el menú interactivo.\n\n"
           "Comandos:\n"
           "  generar  --filas M --columnas N [--prob P] [--semilla S]\n"
           "           [--salida archivo] [--salida-basica archivo] [--forzar]\n"
           "      Ejercicio 1: matriz booleana aleatoria, su matriz básica y densidad.\n\n"
           "  basica   --entrada archivo [--salida archivo] [--forzar]\n"
           "      Reduce una matriz guardada a su matriz básica y calcula la densidad.\n\n"
           "  buscar   [--columnas 7] [--filas 8] [--densidad-min 0.4] [--prob 0.5]\n"
           "           [--min-filas-basica K] [--max-filas-basica K] [--semilla S]\n"
           "           [--intentos 100000] [--salida archivo] [--forzar]\n"
           "      Ejercicio 2.1: busca una matriz básica que cumpla las restricciones.\n\n"
           "  yyc      --entrada archivo [--traza] [--completo] [--repeticiones R]\n"
           "           [--csv archivo] [--latex archivo] [--latex-matriz archivo] [--forzar]\n"
           "      Ejercicios 2.2-2.4: algoritmo YYC; muestra Ψ* y el tiempo acumulado\n"
           "      tras cada fila. --traza explica cada decisión paso a paso.\n"
           "      --completo muestra todos los testores (por defecto, hasta 30 por fila).\n"
           "      --repeticiones toma la mediana de R ejecuciones para la tabla.\n\n"
           "  bt       --entrada archivo [--traza] [--completo] [--repeticiones R]\n"
           "           [--csv archivo] [--latex archivo] [--latex-traza archivo] [--forzar]\n"
           "      Ejercicios 3.1-3.3: algoritmo BT; muestra los testores encontrados,\n"
           "      cuáles son típicos y el tiempo total. --traza explica cada n-uplo α\n"
           "      analizado y cada salto. --repeticiones reporta la mediana de R tiempos.\n\n"
           "  ayuda    Muestra este mensaje.\n\n"
           "Ejercicios 4 y 5: use 'testores_experimentos --ayuda' para generar\n"
           "las matrices B/C y comparar los operadores phi, theta y gamma.\n\n"
           "Formato de archivo: una fila por línea con 0/1 (separadores ' ', ',', ';'\n"
           "opcionales); '#' inicia un comentario.\n";
}

std::string requireOption(const ParsedArguments& args, const std::string& name) {
    const auto it = args.options.find(name);
    if (it == args.options.end()) {
        throw std::invalid_argument("Falta la opción obligatoria " + name);
    }
    return it->second;
}

std::optional<std::string> optionalOption(const ParsedArguments& args, const std::string& name) {
    const auto it = args.options.find(name);
    if (it == args.options.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::size_t sizeOption(const ParsedArguments& args, const std::string& name, std::size_t defaultValue,
                       std::size_t min, std::size_t max) {
    const auto text = optionalOption(args, name);
    if (!text) {
        return defaultValue;
    }
    const auto value = parseInteger(*text, static_cast<long long>(min), static_cast<long long>(max));
    if (!value) {
        throw std::invalid_argument(name + " debe ser un entero entre " + std::to_string(min) + " y " +
                                    std::to_string(max));
    }
    return static_cast<std::size_t>(*value);
}

double realOption(const ParsedArguments& args, const std::string& name, double defaultValue, double min,
                  double max) {
    const auto text = optionalOption(args, name);
    if (!text) {
        return defaultValue;
    }
    const auto value = parseReal(*text, min, max);
    if (!value) {
        throw std::invalid_argument(name + " debe ser un número entre " + std::to_string(min) + " y " +
                                    std::to_string(max));
    }
    return *value;
}

std::uint64_t seedOption(const ParsedArguments& args, CliServices& services) {
    const auto text = optionalOption(args, "--semilla");
    if (!text) {
        return services.random.freshSeed();
    }
    const auto value = parseInteger(*text, 0, std::numeric_limits<long long>::max());
    if (!value) {
        throw std::invalid_argument("--semilla debe ser un entero no negativo");
    }
    return static_cast<std::uint64_t>(*value);
}

std::string describeGeneration(const GenerationRequest& request, std::uint64_t seed) {
    std::ostringstream text;
    text << "Generada: " << request.rows << " x " << request.cols << ", P(1) = " << request.onesProbability
         << ", semilla = " << seed;
    return text.str();
}

void showMatrix(std::ostream& out, const BooleanMatrix& matrix, const std::string& title) {
    if (matrix.rows() <= kMaxPrintedRows) {
        printMatrix(out, matrix, title);
    } else {
        out << "\n--- " << title << " (" << matrix.rows() << " x " << matrix.cols()
            << ") --- (demasiadas filas para mostrarla completa)\n";
    }
}

void showGenerated(std::ostream& out, const application::GeneratedMatrices& generated) {
    showMatrix(out, generated.original, "MATRIZ BOOLEANA ALEATORIA");
    showMatrix(out, generated.reduction.basic, "MATRIZ BÁSICA");
    printReduction(out, generated.original, generated.reduction);
}

// YYC y BT trabajan sobre matrices básicas: si la entrada no lo es se reduce primero
// (el conjunto de testores típicos de una matriz y de su básica coinciden).
BooleanMatrix ensureBasic(std::ostream& out, const BooleanMatrix& matrix) {
    if (domain::isBasicMatrix(matrix)) {
        return matrix;
    }
    out << "\nLa matriz ingresada no es básica; se reduce antes de aplicar el algoritmo.\n";
    const domain::BasicReduction reduction = domain::reduceToBasic(matrix);
    printReduction(out, matrix, reduction);
    return reduction.basic;
}

YycRunResult executeYyc(std::ostream& out, const BooleanMatrix& basic, bool explain, std::size_t repetitions,
                        std::size_t shownSets, application::Clock& clock) {
    application::YycRunOptions options;
    options.solver.recordTrace = explain;

    showMatrix(out, basic, "MATRIZ BÁSICA DE ENTRADA PARA YYC");
    out << "Densidad: " << basic.density() << "\n\n";
    ConsoleYycListener listener(out, basic, explain, shownSets);
    YycRunResult result = application::runYyc(basic, clock, &listener, options);
    if (explain) {
        out << "\n(Nota: con --traza los tiempos incluyen el registro de la traza; para medir use la tabla.)\n";
    }

    if (repetitions > 1 || explain) {
        out << "\nMidiendo tiempos sin traza (" << repetitions << " ejecución/es, se reporta la mediana)...\n";
        result = application::benchmarkYyc(basic, clock, repetitions, options);
    }
    printYycSummaryTable(out, result, shownSets);
    return result;
}

// Mismo criterio que YYC: BT se aplica a la matriz básica.
BtRunResult executeBt(std::ostream& out, const BooleanMatrix& basic, bool explain, std::size_t repetitions,
                      std::size_t shownSets, application::Clock& clock) {
    domain::BtOptions options;
    options.recordTrace = explain;

    showMatrix(out, basic, "MATRIZ BÁSICA DE ENTRADA PARA BT");
    out << "Densidad: " << basic.density() << '\n';
    BtRunResult result = repetitions > 1 ? application::benchmarkBt(basic, clock, repetitions, options)
                                         : application::runBt(basic, clock, options);
    if (explain) {
        printBtTrace(out, basic, result.run);
        if (repetitions == 1) {
            // El tiempo con traza incluye el registro de cada paso; se mide aparte sin ella.
            result.total = application::runBt(basic, clock).total;
        }
    }
    printBtSummary(out, basic, result, shownSets);
    return result;
}

int commandGenerate(const ParsedArguments& args, std::ostream& out, CliServices& services) {
    GenerationRequest request;
    request.rows = sizeOption(args, "--filas", 0, 1, MatrixLimits::kMaxRows);
    request.cols = sizeOption(args, "--columnas", 0, 1, MatrixLimits::kMaxCols);
    if (request.rows == 0 || request.cols == 0) {
        throw std::invalid_argument("Debe indicar --filas y --columnas");
    }
    request.onesProbability = realOption(args, "--prob", 0.5, 0.0, 1.0);
    const std::uint64_t seed = seedOption(args, services);
    auto random = services.random.create(seed);

    const auto generated = application::generateWithBasic(request, *random);
    out << "Semilla utilizada: " << seed << " (úsela con --semilla para reproducir)\n";
    showGenerated(out, generated);

    const bool overwrite = args.flags.count("--forzar") > 0;
    const std::string description = describeGeneration(request, seed);
    if (const auto path = optionalOption(args, "--salida")) {
        services.matrices.save(*path, generated.original, {description}, overwrite);
        out << "\nMatriz original guardada en " << *path << '\n';
    }
    if (const auto path = optionalOption(args, "--salida-basica")) {
        services.matrices.save(*path, generated.reduction.basic, {"Matriz básica. " + description}, overwrite);
        out << "Matriz básica guardada en " << *path << '\n';
    }
    return 0;
}

int commandBasic(const ParsedArguments& args, std::ostream& out, CliServices& services) {
    const std::string input = requireOption(args, "--entrada");
    const BooleanMatrix matrix = services.matrices.load(input);
    const domain::BasicReduction reduction = domain::reduceToBasic(matrix);
    showMatrix(out, matrix, "MATRIZ DE ENTRADA");
    showMatrix(out, reduction.basic, "MATRIZ BÁSICA");
    printReduction(out, matrix, reduction);
    if (const auto path = optionalOption(args, "--salida")) {
        services.matrices.save(*path, reduction.basic, {"Matriz básica de " + input},
                                   args.flags.count("--forzar") > 0);
        out << "\nMatriz básica guardada en " << *path << '\n';
    }
    return 0;
}

int commandSearch(const ParsedArguments& args, std::ostream& out, CliServices& services) {
    GenerationRequest request;
    request.rows = sizeOption(args, "--filas", 8, 1, MatrixLimits::kMaxRows);
    request.cols = sizeOption(args, "--columnas", 7, 1, MatrixLimits::kMaxCols);
    request.onesProbability = realOption(args, "--prob", 0.5, 0.0, 1.0);

    BasicMatrixConstraints constraints;
    constraints.minDensity = realOption(args, "--densidad-min", 0.4, 0.0, 1.0);
    constraints.minRows = sizeOption(args, "--min-filas-basica", 1, 1, MatrixLimits::kMaxRows);
    constraints.maxRows = sizeOption(args, "--max-filas-basica", MatrixLimits::kMaxRows, 1, MatrixLimits::kMaxRows);
    constraints.maxAttempts = sizeOption(args, "--intentos", 100000, 1, 10000000);

    const std::uint64_t seed = seedOption(args, services);
    auto random = services.random.create(seed);
    const auto outcome = application::searchBasicMatrix(request, constraints, *random);
    out << "Semilla utilizada: " << seed << " | intentos: " << outcome.attempts << '\n';
    if (!outcome.found) {
        out << "No se encontró una matriz que cumpla las restricciones. Pruebe más intentos u otra --prob.\n";
        return 2;
    }
    showGenerated(out, *outcome.found);
    if (const auto path = optionalOption(args, "--salida")) {
        std::ostringstream description;
        description << "Matriz básica (Ej. 2.1). " << describeGeneration(request, seed)
                    << ", intento = " << outcome.attempts << ", densidad = " << outcome.found->reduction.basic.density();
        services.matrices.save(*path, outcome.found->reduction.basic, {description.str()},
                                   args.flags.count("--forzar") > 0);
        out << "\nMatriz básica guardada en " << *path << '\n';
    }
    return 0;
}

int commandYyc(const ParsedArguments& args, std::ostream& out, CliServices& services) {
    const std::string input = requireOption(args, "--entrada");
    const std::size_t repetitions = sizeOption(args, "--repeticiones", 1, 1, kMaxRepetitions);
    const bool overwrite = args.flags.count("--forzar") > 0;

    const BooleanMatrix basic = ensureBasic(out, services.matrices.load(input));
    const std::size_t shown = args.flags.count("--completo") > 0 ? kShowAll : kDefaultShownSets;
    const YycRunResult result = executeYyc(out, basic, args.flags.count("--traza") > 0, repetitions, shown, services.clock);

    if (const auto path = optionalOption(args, "--csv")) {
        services.exports.yycCsv(*path, result, overwrite);
        out << "Tabla CSV guardada en " << *path << '\n';
    }
    if (const auto path = optionalOption(args, "--latex")) {
        services.exports.yycLatex(*path, result, overwrite);
        out << "Tabla LaTeX guardada en " << *path << '\n';
    }
    if (const auto path = optionalOption(args, "--latex-matriz")) {
        services.exports.matrixLatex(*path, basic, overwrite);
        out << "Matriz en LaTeX guardada en " << *path << '\n';
    }
    return 0;
}

int commandBt(const ParsedArguments& args, std::ostream& out, CliServices& services) {
    const std::string input = requireOption(args, "--entrada");
    const std::size_t repetitions = sizeOption(args, "--repeticiones", 1, 1, kMaxRepetitions);
    const bool overwrite = args.flags.count("--forzar") > 0;

    const BooleanMatrix basic = ensureBasic(out, services.matrices.load(input));
    const std::size_t shown = args.flags.count("--completo") > 0 ? kShowAll : kDefaultShownSets;
    const bool explain = args.flags.count("--traza") > 0 || args.options.count("--latex-traza") > 0;
    const BtRunResult result = executeBt(out, basic, explain, repetitions, shown, services.clock);

    if (const auto path = optionalOption(args, "--csv")) {
        services.exports.btCsv(*path, result, overwrite);
        out << "Testores encontrados (CSV) guardados en " << *path << '\n';
    }
    if (const auto path = optionalOption(args, "--latex")) {
        services.exports.btLatex(*path, basic, result, overwrite);
        out << "Resumen LaTeX guardado en " << *path << '\n';
    }
    if (const auto path = optionalOption(args, "--latex-traza")) {
        services.exports.btTraceLatex(*path, basic, result.run, overwrite);
        out << "Traza LaTeX guardada en " << *path << '\n';
    }
    return 0;
}

// ----------------------------------------------------------------------------
// Menú interactivo
// ----------------------------------------------------------------------------

class InteractiveSession {
public:
    InteractiveSession(std::istream& in, std::ostream& out, CliServices& dependencies)
        : input_(in, out), out_(out), services(dependencies) {}

    int run() {
        out_ << "=====================================================\n"
                "  Testores típicos: generador de matrices, YYC y BT\n"
                "  Matemáticas Discretas - USFQ\n"
                "=====================================================\n";
        while (true) {
            printMenu();
            const long long option = input_.readInteger("Opción: ", 0, 12);
            if (option == 0) {
                out_ << "Hasta luego.\n";
                return 0;
            }
            try {
                dispatch(option);
            } catch (const InputClosed&) {
                throw;
            } catch (const std::exception& error) {
                out_ << "\n[Error] " << error.what() << '\n';
            }
        }
    }

private:
    void printMenu() {
        out_ << "\n----------------------- MENÚ -----------------------\n"
                " 1. Generar matriz booleana aleatoria y su básica (Ej. 1)\n"
                " 2. Buscar matriz básica con columnas/densidad mínimas (Ej. 2.1)\n"
                " 3. Cargar matriz desde archivo\n"
                " 4. Ingresar matriz manualmente\n"
                " 5. Ver matriz actual, su matriz básica y densidades\n"
                " 6. Ejecutar YYC: Ψ* y tiempo acumulado por fila (Ej. 2.3)\n"
                " 7. Ejecutar YYC paso a paso con explicación (Ej. 2.2)\n"
                " 8. Guardar matriz actual / su básica en archivo\n"
                " 9. Exportar la última tabla YYC (CSV y LaTeX)\n"
                "10. Ejecutar BT: testores, típicos y tiempo total (Ej. 3.2)\n"
                "11. Ejecutar BT paso a paso con explicación (Ej. 3.1)\n"
                "12. Exportar el último resultado BT (CSV y LaTeX)\n"
                " 0. Salir\n";
        if (matrix_) {
            out_ << "Matriz actual: " << matrix_->rows() << " x " << matrix_->cols() << '\n';
        }
    }

    void dispatch(long long option) {
        switch (option) {
            case 1: generate(); break;
            case 2: search(); break;
            case 3: load(); break;
            case 4: typeIn(); break;
            case 5: show(); break;
            case 6: yyc(false); break;
            case 7: yyc(true); break;
            case 8: save(); break;
            case 9: exportLast(); break;
            case 10: bt(false); break;
            case 11: bt(true); break;
            case 12: exportLastBt(); break;
            default: break;
        }
    }

    std::uint64_t askSeed() {
        const std::string text = input_.readLine("Semilla (Enter = aleatoria): ");
        if (text.empty()) {
            return services.random.freshSeed();
        }
        const auto value = parseInteger(text, 0, std::numeric_limits<long long>::max());
        if (!value) {
            throw std::invalid_argument("La semilla debe ser un entero no negativo");
        }
        return static_cast<std::uint64_t>(*value);
    }

    GenerationRequest askRequest(std::size_t defaultRows, std::size_t defaultCols) {
        GenerationRequest request;
        request.rows = input_.readSize("Número de filas (1-" + std::to_string(MatrixLimits::kMaxRows) + ", sugerido " +
                                           std::to_string(defaultRows) + "): ",
                                       1, MatrixLimits::kMaxRows);
        request.cols = input_.readSize("Número de columnas (1-" + std::to_string(MatrixLimits::kMaxCols) +
                                           ", sugerido " + std::to_string(defaultCols) + "): ",
                                       1, MatrixLimits::kMaxCols);
        const auto p = parseReal(input_.readLineOr("Probabilidad de 1 por entrada [0.5]: ", "0.5"), 0.0, 1.0);
        if (!p) {
            throw std::invalid_argument("La probabilidad debe estar en [0, 1]");
        }
        request.onesProbability = *p;
        return request;
    }

    void setMatrix(BooleanMatrix matrix) {
        matrix_ = std::move(matrix);
        lastResult_.reset();
        lastBt_.reset();
        lastBtMatrix_.reset();
    }

    void generate() {
        const GenerationRequest request = askRequest(100, 10);
        const std::uint64_t seed = askSeed();
        auto random = services.random.create(seed);
        auto generated = application::generateWithBasic(request, *random);
        out_ << "Semilla utilizada: " << seed << '\n';
        showGenerated(out_, generated);
        setMatrix(generated.original);
    }

    void search() {
        const GenerationRequest request = askRequest(8, 7);
        BasicMatrixConstraints constraints;
        const auto density = parseReal(input_.readLineOr("Densidad mínima de la básica [0.4]: ", "0.4"), 0.0, 1.0);
        if (!density) {
            throw std::invalid_argument("La densidad debe estar en [0, 1]");
        }
        constraints.minDensity = *density;
        constraints.minRows = input_.readSize("Mínimo de filas de la básica [1-" + std::to_string(request.rows) + "]: ",
                                              1, request.rows);
        const std::uint64_t seed = askSeed();
        auto random = services.random.create(seed);
        const auto outcome = application::searchBasicMatrix(request, constraints, *random);
        out_ << "Semilla utilizada: " << seed << " | intentos: " << outcome.attempts << '\n';
        if (!outcome.found) {
            out_ << "No se encontró una matriz que cumpla las restricciones.\n";
            return;
        }
        showGenerated(out_, *outcome.found);
        setMatrix(outcome.found->reduction.basic);
        out_ << "\nLa matriz básica encontrada es ahora la matriz actual.\n";
    }

    void load() {
        const std::string path = input_.readLine("Ruta del archivo: ");
        setMatrix(services.matrices.load(path));
        printMatrix(out_, *matrix_, "MATRIZ CARGADA");
    }

    void typeIn() {
        const std::size_t rows = input_.readSize("Número de filas (1-50): ", 1, 50);
        const std::size_t cols = input_.readSize("Número de columnas (1-64): ", 1, 64);
        out_ << "Escriba cada fila con " << cols << " valores 0/1 (ej. 1 0 1 1 o 1011).\n";
        std::ostringstream text;
        for (std::size_t i = 0; i < rows; ++i) {
            while (true) {
                const std::string line = input_.readLine("r" + std::to_string(i + 1) + ": ");
                try {
                    std::istringstream single(line);
                    if (services.matrices.parse(single).cols() == cols) {
                        text << line << '\n';
                        break;
                    }
                    out_ << "  Se esperaban " << cols << " valores.\n";
                } catch (const std::exception& error) {
                    out_ << "  " << error.what() << '\n';
                }
            }
        }
        std::istringstream all(text.str());
        setMatrix(services.matrices.parse(all));
        printMatrix(out_, *matrix_, "MATRIZ INGRESADA");
    }

    const BooleanMatrix& requireMatrix() const {
        if (!matrix_) {
            throw std::logic_error("Primero genere, cargue o ingrese una matriz (opciones 1-4)");
        }
        return *matrix_;
    }

    void show() {
        const BooleanMatrix& matrix = requireMatrix();
        const domain::BasicReduction reduction = domain::reduceToBasic(matrix);
        showMatrix(out_, matrix, "MATRIZ ACTUAL");
        showMatrix(out_, reduction.basic, "MATRIZ BÁSICA");
        printReduction(out_, matrix, reduction);
    }

    void yyc(bool explain) {
        const BooleanMatrix basic = ensureBasic(out_, requireMatrix());
        const std::size_t repetitions =
            explain ? 1 : input_.readSize("Repeticiones para medir tiempos (1 = una sola corrida): ", 1, kMaxRepetitions);
        lastResult_ = executeYyc(out_, basic, explain, repetitions, kDefaultShownSets, services.clock);
    }

    void bt(bool explain) {
        const BooleanMatrix basic = ensureBasic(out_, requireMatrix());
        const std::size_t repetitions =
            explain ? 1 : input_.readSize("Repeticiones para medir el tiempo (1 = una sola corrida): ", 1, kMaxRepetitions);
        lastBt_ = executeBt(out_, basic, explain, repetitions, kDefaultShownSets, services.clock);
        lastBtMatrix_ = basic;
    }

    void exportLastBt() {
        if (!lastBt_ || !lastBtMatrix_) {
            throw std::logic_error("Primero ejecute BT (opción 10 u 11)");
        }
        const std::string base = input_.readLine("Ruta base (sin extensión), ej. resultados/bt_A: ");
        const bool overwrite = input_.readYesNo("¿Sobrescribir si existen?", false);
        services.exports.btCsv(base + ".csv", *lastBt_, overwrite);
        services.exports.btLatex(base + ".tex", *lastBtMatrix_, *lastBt_, overwrite);
        out_ << "Exportado: " << base << ".csv y " << base << ".tex\n";
        if (!lastBt_->run.trace.empty()) {
            services.exports.btTraceLatex(base + "_traza.tex", *lastBtMatrix_, lastBt_->run, overwrite);
            out_ << "Exportado: " << base << "_traza.tex\n";
        }
    }

    void save() {
        const BooleanMatrix& matrix = requireMatrix();
        const bool basic = input_.readYesNo("¿Guardar la matriz básica en lugar de la actual?", false);
        const std::string path = input_.readLine("Ruta de destino: ");
        bool overwrite = false;
        if (services.matrices.exists(path)) {
            overwrite = input_.readYesNo("El archivo existe. ¿Sobrescribir?", false);
            if (!overwrite) {
                out_ << "No se guardó nada.\n";
                return;
            }
        }
        services.matrices.save(path, basic ? domain::toBasicMatrix(matrix) : matrix,
                                   {basic ? "Matriz básica" : "Matriz"}, overwrite);
        out_ << "Guardada en " << path << '\n';
    }

    void exportLast() {
        if (!lastResult_) {
            throw std::logic_error("Primero ejecute YYC (opción 6 o 7)");
        }
        const std::string base = input_.readLine("Ruta base (sin extensión), ej. resultados/yyc_A: ");
        const bool overwrite = input_.readYesNo("¿Sobrescribir si existen?", false);
        services.exports.yycCsv(base + ".csv", *lastResult_, overwrite);
        services.exports.yycLatex(base + ".tex", *lastResult_, overwrite);
        out_ << "Exportado: " << base << ".csv y " << base << ".tex\n";
    }

    ConsoleInput input_;
    std::ostream& out_;
    CliServices& services;
    std::optional<BooleanMatrix> matrix_;
    std::optional<YycRunResult> lastResult_;
    std::optional<BtRunResult> lastBt_;
    std::optional<BooleanMatrix> lastBtMatrix_;
};

}  // namespace

ParsedArguments parseArguments(const std::vector<std::string>& args) {
    ParsedArguments parsed;
    if (args.empty()) {
        parsed.command = "menu";
        return parsed;
    }
    parsed.command = args.front();
    if (parsed.command == "-h" || parsed.command == "--ayuda" || parsed.command == "--help") {
        parsed.command = "ayuda";
    }
    const auto allowed = kAllowedOptions.find(parsed.command);
    if (allowed == kAllowedOptions.end()) {
        throw std::invalid_argument("Comando desconocido: " + parsed.command);
    }
    for (std::size_t i = 1; i < args.size(); ++i) {
        const std::string& key = args[i];
        if (allowed->second.count(key) == 0) {
            throw std::invalid_argument("Opción no válida para '" + parsed.command + "': " + key);
        }
        if (kFlags.count(key) > 0) {
            if (!parsed.flags.insert(key).second) {
                throw std::invalid_argument("Opción repetida: " + key);
            }
            continue;
        }
        if (i + 1 >= args.size()) {
            throw std::invalid_argument("La opción " + key + " requiere un valor");
        }
        if (!parsed.options.emplace(key, args[++i]).second) {
            throw std::invalid_argument("Opción repetida: " + key);
        }
    }
    return parsed;
}

int runApplication(const std::vector<std::string>& args, std::istream& in, std::ostream& out, std::ostream& err,
                   CliServices& services) {
    try {
        const ParsedArguments parsed = parseArguments(args);
        if (parsed.command == "menu") {
            return InteractiveSession(in, out, services).run();
        }
        if (parsed.command == "ayuda") {
            printUsage(out);
            return 0;
        }
        if (parsed.command == "generar") {
            return commandGenerate(parsed, out, services);
        }
        if (parsed.command == "basica") {
            return commandBasic(parsed, out, services);
        }
        if (parsed.command == "buscar") {
            return commandSearch(parsed, out, services);
        }
        if (parsed.command == "bt") {
            return commandBt(parsed, out, services);
        }
        return commandYyc(parsed, out, services);
    } catch (const InputClosed&) {
        out << "\nEntrada cerrada. Fin del programa.\n";
        return 0;
    } catch (const std::invalid_argument& error) {
        err << "Error: " << error.what() << "\nUse 'testores ayuda' para ver las opciones.\n";
        return 1;
    } catch (const std::exception& error) {
        err << "Error: " << error.what() << '\n';
        return 1;
    }
}

}  // namespace testores::presentation
