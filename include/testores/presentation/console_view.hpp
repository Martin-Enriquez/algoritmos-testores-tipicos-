#pragma once

#include <chrono>
#include <iosfwd>
#include <string>

#include "testores/application/ports.hpp"
#include "testores/application/yyc_runner.hpp"
#include "testores/domain/basic_matrix.hpp"
#include "testores/domain/boolean_matrix.hpp"

namespace testores::presentation {

std::string formatDuration(std::chrono::nanoseconds ns);

// Matriz con encabezado de rasgos (x1 ... xn) y etiquetas de fila (r1 ... rm).
void printMatrix(std::ostream& out, const domain::BooleanMatrix& matrix, const std::string& title);

// Resumen de la reducción: qué filas se eliminaron y por qué, y densidades.
void printReduction(std::ostream& out, const domain::BooleanMatrix& original, const domain::BasicReduction& reduction);

// Cantidad de conjuntos que se muestran por fila salvo que se pida verlos todos.
constexpr std::size_t kDefaultShownSets = 30;
constexpr std::size_t kShowAll = static_cast<std::size_t>(-1);

// Los `limit` primeros conjuntos en orden canónico y "... y N más" si sobran.
std::string formatFeatureSetsLimited(const std::vector<domain::ColumnSet>& sets, std::size_t limit);

void printYycSummaryTable(std::ostream& out, const application::YycRunResult& result, std::size_t limit);

// Muestra en vivo, después de cada fila, Ψ* y el tiempo acumulado (Ej. 2.3).
// Con `explain` = true también narra cada decisión del algoritmo (Ej. 2.2).
class ConsoleYycListener final : public application::YycProgressListener {
public:
    ConsoleYycListener(std::ostream& out, const domain::BooleanMatrix& matrix, bool explain, std::size_t limit)
        : out_(out), matrix_(matrix), explain_(explain), limit_(limit) {}

    void onRowProcessed(const application::YycRowProgress& progress) override;

private:
    void explainStep(const application::YycRowProgress& progress);

    std::ostream& out_;
    const domain::BooleanMatrix& matrix_;
    bool explain_;
    std::size_t limit_;
};

}  // namespace testores::presentation
