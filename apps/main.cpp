#include <iostream>
#include <string>
#include <vector>

#include "testores/presentation/cli.hpp"
#include "testores/infrastructure/cli_adapters.hpp"
#include "testores/infrastructure/system_adapters.hpp"

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
    testores::infrastructure::FileMatrixRepository matrices;
    testores::infrastructure::FileResultExporter exports;
    testores::infrastructure::MersenneRandomFactory random;
    testores::infrastructure::SteadyClock clock;
    testores::presentation::CliServices services{matrices, exports, random, clock};
    return testores::presentation::runApplication(args, std::cin, std::cout, std::cerr, services);
}
