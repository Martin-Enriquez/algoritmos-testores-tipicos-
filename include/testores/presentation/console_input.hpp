#pragma once

#include <cstddef>
#include <iosfwd>
#include <optional>
#include <stdexcept>
#include <string>

namespace testores::presentation {

// Se lanza cuando la entrada estándar se cierra (EOF); permite terminar limpio.
class InputClosed : public std::runtime_error {
public:
    InputClosed() : std::runtime_error("Entrada cerrada") {}
};

// Conversión estricta: toda la cadena debe ser un número válido dentro del rango.
std::optional<long long> parseInteger(const std::string& text, long long min, long long max);
std::optional<double> parseReal(const std::string& text, double min, double max);

// Lectura interactiva y validada. Ante una entrada inválida vuelve a preguntar;
// nunca deja el flujo en estado de error como `std::cin >> int`.
class ConsoleInput {
public:
    ConsoleInput(std::istream& in, std::ostream& out) : in_(in), out_(out) {}

    std::string readLine(const std::string& prompt);
    long long readInteger(const std::string& prompt, long long min, long long max);
    std::size_t readSize(const std::string& prompt, std::size_t min, std::size_t max);
    double readReal(const std::string& prompt, double min, double max);
    bool readYesNo(const std::string& prompt, bool defaultValue);
    // Devuelve defaultValue si el usuario presiona Enter sin escribir nada.
    std::string readLineOr(const std::string& prompt, const std::string& defaultValue);

private:
    static constexpr std::size_t kMaxLineLength = 4096;

    std::istream& in_;
    std::ostream& out_;
};

}  // namespace testores::presentation
