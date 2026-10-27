#include <sstream>
#include <stdexcept>

#include "test_framework.hpp"
#include "testores/presentation/cli.hpp"
#include "testores/presentation/console_input.hpp"

using namespace testores::presentation;

TEST(strict_number_parsing) {
    CHECK(parseInteger("42", 0, 100) == std::optional<long long>(42));
    CHECK(!parseInteger("42abc", 0, 100));
    CHECK(!parseInteger("", 0, 100));
    CHECK(!parseInteger("101", 0, 100));
    CHECK(!parseInteger("99999999999999999999999", 0, 100));
    CHECK(parseReal("0,4", 0, 1) == std::optional<double>(0.4));
    CHECK(!parseReal("nan", 0, 1));
    CHECK(!parseReal("1.5", 0, 1));
}

TEST(console_input_retries_until_valid) {
    std::istringstream in("abc\n-3\n7\n");
    std::ostringstream out;
    ConsoleInput input(in, out);
    CHECK_EQ(input.readInteger("n: ", 1, 10), 7LL);
    CHECK(out.str().find("Valor inválido") != std::string::npos);
}

TEST(console_input_signals_eof) {
    std::istringstream in("");
    std::ostringstream out;
    ConsoleInput input(in, out);
    CHECK_THROWS(input.readInteger("n: ", 1, 10), InputClosed);
}

TEST(argument_parsing_validates_options) {
    const auto parsed = parseArguments({"yyc", "--entrada", "a.txt", "--traza"});
    CHECK_EQ(parsed.command, std::string("yyc"));
    CHECK_EQ(parsed.options.at("--entrada"), std::string("a.txt"));
    CHECK(parsed.flags.count("--traza") == 1);
    CHECK_THROWS(parseArguments({"yyc", "--desconocida", "1"}), std::invalid_argument);
    CHECK_THROWS(parseArguments({"yyc", "--entrada"}), std::invalid_argument);
    CHECK_THROWS(parseArguments({"yyc", "--entrada", "a", "--entrada", "b"}), std::invalid_argument);
    CHECK_THROWS(parseArguments({"borrar"}), std::invalid_argument);
    CHECK_EQ(parseArguments({}).command, std::string("menu"));
}

TEST(generate_command_end_to_end) {
    std::istringstream in;
    std::ostringstream out;
    std::ostringstream err;
    const int code = runApplication({"generar", "--filas", "100", "--columnas", "10", "--semilla", "1"}, in, out, err);
    CHECK_EQ(code, 0);
    CHECK(out.str().find("MATRIZ BÁSICA") != std::string::npos);
    CHECK(out.str().find("Densidad matriz básica") != std::string::npos);
}

TEST(invalid_command_line_returns_error_code) {
    std::istringstream in;
    std::ostringstream out;
    std::ostringstream err;
    CHECK_EQ(runApplication({"generar", "--filas", "-5", "--columnas", "3"}, in, out, err), 1);
    CHECK(err.str().find("--filas") != std::string::npos);
}

TEST(interactive_session_generates_and_runs_yyc) {
    // 1: generar 6x5 con p por defecto y semilla 3; 6: YYC con 1 repetición; 0: salir.
    std::istringstream in("1\n6\n5\n\n3\n6\n1\n0\n");
    std::ostringstream out;
    std::ostringstream err;
    CHECK_EQ(runApplication({}, in, out, err), 0);
    CHECK(out.str().find("Testores típicos de la matriz") != std::string::npos);
    CHECK(out.str().find("tiempo acumulado") != std::string::npos);
}

TEST(interactive_session_survives_eof) {
    std::istringstream in("1\n");
    std::ostringstream out;
    std::ostringstream err;
    CHECK_EQ(runApplication({}, in, out, err), 0);
}
