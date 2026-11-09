#include <exception>
#include <iostream>

#include "test_framework.hpp"

int main() {
    int failed = 0;
    for (const auto& test : testfw::registry()) {
        try {
            test.body();
            std::cout << "[ OK ] " << test.name << '\n';
        } catch (const testfw::Failure& failure) {
            ++failed;
            std::cout << "[FALLA] " << test.name << "\n        " << failure.message << '\n';
        } catch (const std::exception& error) {
            ++failed;
            std::cout << "[FALLA] " << test.name << "\n        excepción inesperada: " << error.what() << '\n';
        }
    }
    std::cout << '\n' << (testfw::registry().size() - static_cast<std::size_t>(failed)) << '/'
              << testfw::registry().size() << " pruebas correctas\n";
    return failed == 0 ? 0 : 1;
}
