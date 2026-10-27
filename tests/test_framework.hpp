#pragma once

// Mini framework de pruebas sin dependencias externas.

#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace testfw {

struct TestCase {
    std::string name;
    std::function<void()> body;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

struct Registrar {
    Registrar(const char* name, std::function<void()> body) { registry().push_back({name, std::move(body)}); }
};

struct Failure {
    std::string message;
};

inline void fail(const char* file, int line, const std::string& message) {
    std::ostringstream out;
    out << file << ':' << line << ": " << message;
    throw Failure{out.str()};
}

}  // namespace testfw

#define TF_CONCAT_INNER(a, b) a##b
#define TF_CONCAT(a, b) TF_CONCAT_INNER(a, b)

#define TEST(name)                                                                   \
    static void TF_CONCAT(test_fn_, name)();                                         \
    static const testfw::Registrar TF_CONCAT(test_reg_, name)(#name, TF_CONCAT(test_fn_, name)); \
    static void TF_CONCAT(test_fn_, name)()

#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) testfw::fail(__FILE__, __LINE__, "CHECK(" #cond ")"); \
    } while (false)

#define CHECK_EQ(a, b)                                                                     \
    do {                                                                                   \
        const auto& tf_a = (a);                                                            \
        const auto& tf_b = (b);                                                            \
        if (!(tf_a == tf_b)) {                                                             \
            std::ostringstream tf_out;                                                     \
            tf_out << "CHECK_EQ(" #a ", " #b "): " << tf_a << " != " << tf_b;              \
            testfw::fail(__FILE__, __LINE__, tf_out.str());                                \
        }                                                                                  \
    } while (false)

#define CHECK_THROWS(expr, type)                                                            \
    do {                                                                                    \
        bool tf_thrown = false;                                                             \
        try {                                                                               \
            (void)(expr);                                                                   \
        } catch (const type&) {                                                             \
            tf_thrown = true;                                                               \
        }                                                                                   \
        if (!tf_thrown) testfw::fail(__FILE__, __LINE__, "se esperaba excepción " #type); \
    } while (false)
