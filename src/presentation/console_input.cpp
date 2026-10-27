#include "testores/presentation/console_input.hpp"

#include <charconv>
#include <cmath>
#include <istream>
#include <ostream>

namespace testores::presentation {

namespace {

std::string trim(const std::string& text) {
    const auto begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }
    const auto end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

}  // namespace

std::optional<long long> parseInteger(const std::string& text, long long min, long long max) {
    const std::string clean = trim(text);
    long long value = 0;
    const char* first = clean.data();
    const char* last = clean.data() + clean.size();
    const auto [ptr, ec] = std::from_chars(first, last, value);
    if (clean.empty() || ec != std::errc() || ptr != last || value < min || value > max) {
        return std::nullopt;
    }
    return value;
}

std::optional<double> parseReal(const std::string& text, double min, double max) {
    std::string clean = trim(text);
    for (char& c : clean) {
        if (c == ',') {
            c = '.';  // se acepta coma decimal
        }
    }
    double value = 0.0;
    const char* first = clean.data();
    const char* last = clean.data() + clean.size();
    const auto [ptr, ec] = std::from_chars(first, last, value);
    if (clean.empty() || ec != std::errc() || ptr != last || !std::isfinite(value) || value < min || value > max) {
        return std::nullopt;
    }
    return value;
}

std::string ConsoleInput::readLine(const std::string& prompt) {
    while (true) {
        out_ << prompt << std::flush;
        std::string line;
        if (!std::getline(in_, line)) {
            throw InputClosed();
        }
        if (line.size() > kMaxLineLength) {
            out_ << "  Entrada demasiado larga (máximo " << kMaxLineLength << " caracteres).\n";
            continue;
        }
        return trim(line);
    }
}

std::string ConsoleInput::readLineOr(const std::string& prompt, const std::string& defaultValue) {
    const std::string line = readLine(prompt);
    return line.empty() ? defaultValue : line;
}

long long ConsoleInput::readInteger(const std::string& prompt, long long min, long long max) {
    while (true) {
        if (auto value = parseInteger(readLine(prompt), min, max)) {
            return *value;
        }
        out_ << "  Valor inválido: ingrese un entero entre " << min << " y " << max << ".\n";
    }
}

std::size_t ConsoleInput::readSize(const std::string& prompt, std::size_t min, std::size_t max) {
    return static_cast<std::size_t>(readInteger(prompt, static_cast<long long>(min), static_cast<long long>(max)));
}

double ConsoleInput::readReal(const std::string& prompt, double min, double max) {
    while (true) {
        if (auto value = parseReal(readLine(prompt), min, max)) {
            return *value;
        }
        out_ << "  Valor inválido: ingrese un número entre " << min << " y " << max << ".\n";
    }
}

bool ConsoleInput::readYesNo(const std::string& prompt, bool defaultValue) {
    while (true) {
        const std::string answer = readLine(prompt + (defaultValue ? " [S/n]: " : " [s/N]: "));
        if (answer.empty()) {
            return defaultValue;
        }
        if (answer == "s" || answer == "S" || answer == "si" || answer == "sí") {
            return true;
        }
        if (answer == "n" || answer == "N" || answer == "no") {
            return false;
        }
        out_ << "  Responda 's' o 'n'.\n";
    }
}

}  // namespace testores::presentation
