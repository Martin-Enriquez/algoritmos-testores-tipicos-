#pragma once

#include <filesystem>
#include <fstream>

namespace testores::infrastructure {

// Abre un archivo para escritura creando los directorios intermedios.
// Lanza std::runtime_error si ya existe y overwrite = false, o si no se puede crear.
std::ofstream openOutputFile(const std::filesystem::path& path, bool overwrite);

// Lanza std::runtime_error si hubo un error de escritura.
void closeOutputFile(std::ofstream& file, const std::filesystem::path& path);

}  // namespace testores::infrastructure
