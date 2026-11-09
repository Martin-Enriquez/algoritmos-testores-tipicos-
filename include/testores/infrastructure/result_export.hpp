#pragma once

#include <filesystem>
#include <iosfwd>

#include "testores/application/bt_runner.hpp"
#include "testores/application/yyc_runner.hpp"
#include "testores/domain/boolean_matrix.hpp"

namespace testores::infrastructure {

// Tabla del Ejercicio 2.4: fila procesada | testores típicos | tiempo acumulado.
void writeYycCsv(std::ostream& output, const application::YycRunResult& result);
void writeYycLatexTable(std::ostream& output, const application::YycRunResult& result);

// Ejercicio 3: testores encontrados por BT (orden de BT, n-uplo, conjunto, típico).
void writeBtCsv(std::ostream& output, const application::BtRunResult& result);
// Ejercicio 3.3: testores típicos, conteos y tiempo total.
void writeBtSummaryLatex(std::ostream& output, const domain::BooleanMatrix& matrix,
                         const application::BtRunResult& result);
// Ejercicio 3.1: un renglón por n-uplo analizado (requiere traza).
void writeBtTraceLatex(std::ostream& output, const domain::BooleanMatrix& matrix, const domain::BtResult& result);

void exportBtCsv(const std::filesystem::path& path, const application::BtRunResult& result, bool overwrite);
void exportBtSummaryLatex(const std::filesystem::path& path, const domain::BooleanMatrix& matrix,
                          const application::BtRunResult& result, bool overwrite);
void exportBtTraceLatex(const std::filesystem::path& path, const domain::BooleanMatrix& matrix,
                        const domain::BtResult& result, bool overwrite);

// Matriz como entorno pmatrix de LaTeX (para el reporte).
void writeMatrixLatex(std::ostream& output, const domain::BooleanMatrix& matrix);

void exportYycCsv(const std::filesystem::path& path, const application::YycRunResult& result, bool overwrite);
void exportYycLatexTable(const std::filesystem::path& path, const application::YycRunResult& result,
                         bool overwrite);

}  // namespace testores::infrastructure
