#pragma once

#include <cstddef>
#include <filesystem>
#include <iosfwd>
#include <stdexcept>
#include <string>
#include <vector>

#include "testores/domain/boolean_matrix.hpp"

// Formato de texto para matrices booleanas:
//   - una fila por línea, entradas '0' o '1';
//   - separadores opcionales: espacio, tabulador, ',' o ';';
//   - líneas vacías y comentarios (desde '#') se ignoran.
namespace testores::infrastructure {

class MatrixFormatError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

struct FileLimits {
    static constexpr std::uintmax_t kMaxFileBytes = 16U * 1024U * 1024U;
};

// Lanza MatrixFormatError (con número de línea) ante cualquier contenido inválido.
domain::BooleanMatrix parseMatrix(std::istream& input);

// Lanza MatrixFormatError si el archivo no existe, no es regular, supera
// FileLimits::kMaxFileBytes o su contenido es inválido.
domain::BooleanMatrix loadMatrix(const std::filesystem::path& path);

// Escribe la matriz con un encabezado de comentarios. Si el archivo existe y
// overwrite = false lanza std::runtime_error (no se sobrescribe nada en silencio).
void saveMatrix(const std::filesystem::path& path, const domain::BooleanMatrix& matrix,
                const std::vector<std::string>& headerComments, bool overwrite);

void writeMatrix(std::ostream& output, const domain::BooleanMatrix& matrix,
                 const std::vector<std::string>& headerComments);

}  // namespace testores::infrastructure
