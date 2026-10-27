#include "testores/infrastructure/matrix_file.hpp"

#include <fstream>
#include <istream>
#include <ostream>
#include <utility>

#include "testores/domain/column_set.hpp"
#include "testores/infrastructure/output_file.hpp"

namespace testores::infrastructure {

using domain::BooleanMatrix;
using domain::ColumnSet;
using domain::MatrixLimits;

namespace {

bool isSeparator(char c) { return c == ' ' || c == '\t' || c == ',' || c == ';' || c == '\r'; }

[[noreturn]] void fail(std::size_t lineNumber, const std::string& message) {
    throw MatrixFormatError("Línea " + std::to_string(lineNumber) + ": " + message);
}

// Elimina saltos de línea para que un comentario no pueda inyectar filas.
std::string sanitizeComment(const std::string& text) {
    std::string out;
    for (char c : text) {
        out += (c == '\n' || c == '\r') ? ' ' : c;
    }
    return out;
}

}  // namespace

BooleanMatrix parseMatrix(std::istream& input) {
    std::vector<ColumnSet> rows;
    std::size_t cols = 0;
    std::size_t lineNumber = 0;
    std::string line;

    while (std::getline(input, line)) {
        ++lineNumber;
        // Marca de orden de bytes UTF-8 que agregan algunos editores de Windows.
        if (lineNumber == 1 && line.rfind("\xEF\xBB\xBF", 0) == 0) {
            line.erase(0, 3);
        }
        const std::size_t comment = line.find('#');
        if (comment != std::string::npos) {
            line.erase(comment);
        }

        std::vector<bool> values;
        for (char c : line) {
            if (c == '0' || c == '1') {
                values.push_back(c == '1');
                if (values.size() > MatrixLimits::kMaxCols) {
                    fail(lineNumber, "más de " + std::to_string(MatrixLimits::kMaxCols) + " columnas");
                }
            } else if (!isSeparator(c)) {
                fail(lineNumber, "carácter no permitido (sólo se aceptan 0, 1 y separadores)");
            }
        }
        if (values.empty()) {
            continue;
        }
        if (rows.empty()) {
            cols = values.size();
        } else if (values.size() != cols) {
            fail(lineNumber, "se esperaban " + std::to_string(cols) + " columnas y hay " +
                                 std::to_string(values.size()));
        }
        if (rows.size() == MatrixLimits::kMaxRows) {
            fail(lineNumber, "más de " + std::to_string(MatrixLimits::kMaxRows) + " filas");
        }
        ColumnSet row(cols);
        for (std::size_t j = 0; j < cols; ++j) {
            row.set(j, values[j]);
        }
        rows.push_back(std::move(row));
    }
    if (input.bad()) {
        throw MatrixFormatError("Error de lectura");
    }
    if (rows.empty()) {
        throw MatrixFormatError("El archivo no contiene ninguna fila");
    }
    return BooleanMatrix(cols, std::move(rows));
}

BooleanMatrix loadMatrix(const std::filesystem::path& path) {
    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec)) {
        throw MatrixFormatError("No existe el archivo o no es un archivo regular: " + path.string());
    }
    const std::uintmax_t size = std::filesystem::file_size(path, ec);
    if (ec) {
        throw MatrixFormatError("No se pudo determinar el tamaño de " + path.string());
    }
    if (size > FileLimits::kMaxFileBytes) {
        throw MatrixFormatError("El archivo supera el tamaño máximo permitido (16 MiB)");
    }
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw MatrixFormatError("No se pudo abrir " + path.string());
    }
    return parseMatrix(file);
}

void writeMatrix(std::ostream& output, const BooleanMatrix& matrix, const std::vector<std::string>& headerComments) {
    for (const std::string& comment : headerComments) {
        output << "# " << sanitizeComment(comment) << '\n';
    }
    for (const ColumnSet& row : matrix.rowSets()) {
        for (std::size_t j = 0; j < matrix.cols(); ++j) {
            if (j > 0) {
                output << ' ';
            }
            output << (row.test(j) ? '1' : '0');
        }
        output << '\n';
    }
}

void saveMatrix(const std::filesystem::path& path, const BooleanMatrix& matrix,
                const std::vector<std::string>& headerComments, bool overwrite) {
    std::ofstream file = openOutputFile(path, overwrite);
    writeMatrix(file, matrix, headerComments);
    closeOutputFile(file, path);
}

}  // namespace testores::infrastructure
