#include "testores/infrastructure/output_file.hpp"

#include <stdexcept>
#include <system_error>

namespace testores::infrastructure {

std::ofstream openOutputFile(const std::filesystem::path& path, bool overwrite) {
    std::error_code ec;
    if (!overwrite && std::filesystem::exists(path, ec)) {
        throw std::runtime_error("El archivo ya existe (use la opción de sobrescribir): " + path.string());
    }
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path(), ec);
    }
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        throw std::runtime_error("No se pudo crear " + path.string());
    }
    return file;
}

void closeOutputFile(std::ofstream& file, const std::filesystem::path& path) {
    file.flush();
    if (!file) {
        throw std::runtime_error("Error al escribir " + path.string());
    }
    file.close();
}

}  // namespace testores::infrastructure
