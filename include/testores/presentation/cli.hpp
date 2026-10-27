#pragma once

#include <iosfwd>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace testores::presentation {

struct ParsedArguments {
    std::string command;
    std::map<std::string, std::string> options;  // --clave valor
    std::set<std::string> flags;                 // --bandera
};

// Lanza std::invalid_argument ante opciones desconocidas, repetidas o sin valor.
ParsedArguments parseArguments(const std::vector<std::string>& args);

// Punto de entrada de la aplicación. Sin argumentos abre el menú interactivo.
// Devuelve el código de salida del proceso.
int runApplication(const std::vector<std::string>& args, std::istream& in, std::ostream& out, std::ostream& err);

}  // namespace testores::presentation
