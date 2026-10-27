#pragma once

#include <filesystem>
#include <iosfwd>

#include "testores/application/yyc_runner.hpp"
#include "testores/domain/boolean_matrix.hpp"

namespace testores::infrastructure {

// Tabla del Ejercicio 2.4: fila procesada | testores típicos | tiempo acumulado.
void writeYycCsv(std::ostream& output, const application::YycRunResult& result);
void writeYycLatexTable(std::ostream& output, const application::YycRunResult& result);

// Matriz como entorno pmatrix de LaTeX (para el reporte).
void writeMatrixLatex(std::ostream& output, const domain::BooleanMatrix& matrix);

void exportYycCsv(const std::filesystem::path& path, const application::YycRunResult& result, bool overwrite);
void exportYycLatexTable(const std::filesystem::path& path, const application::YycRunResult& result,
                         bool overwrite);

}  // namespace testores::infrastructure
