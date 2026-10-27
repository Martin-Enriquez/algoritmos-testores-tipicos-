#include <iostream>
#include <string>
#include <vector>

#include "testores/presentation/cli.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char* argv[]) {
#ifdef _WIN32
    // Mostrar correctamente tildes y símbolos (Ψ, τ, µ) en la consola de Windows.
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    std::ios::sync_with_stdio(false);
    const std::vector<std::string> args(argv + 1, argv + argc);
    return testores::presentation::runApplication(args, std::cin, std::cout, std::cerr);
}
