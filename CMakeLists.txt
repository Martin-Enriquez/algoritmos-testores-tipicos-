cmake_minimum_required(VERSION 3.16)
project(testores_tipicos VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Tipo de compilación" FORCE)
endif()

option(TESTORES_BUILD_TESTS "Compilar las pruebas unitarias" ON)
option(TESTORES_SANITIZERS "Compilar con AddressSanitizer y UBSan (GCC/Clang en Linux/macOS)" OFF)

# ---------------------------------------------------------------------------
# Advertencias y endurecimiento (hardening) comunes a todos los objetivos
# ---------------------------------------------------------------------------
add_library(testores_options INTERFACE)
if(MSVC)
    target_compile_options(testores_options INTERFACE /W4 /permissive- /sdl /utf-8 /EHsc)
    target_compile_definitions(testores_options INTERFACE NOMINMAX WIN32_LEAN_AND_MEAN)
else()
    target_compile_options(testores_options INTERFACE
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
        -Wnon-virtual-dtor -Wold-style-cast -Wnull-dereference
        -fstack-protector-strong)
    # Comprobación de límites en contenedores de libstdc++ (coste bajo).
    target_compile_definitions(testores_options INTERFACE _GLIBCXX_ASSERTIONS
        $<$<NOT:$<CONFIG:Debug>>:_FORTIFY_SOURCE=2>)
    if(TESTORES_SANITIZERS)
        target_compile_options(testores_options INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(testores_options INTERFACE -fsanitize=address,undefined)
    endif()
endif()

# ---------------------------------------------------------------------------
# Capas (arquitectura limpia). Cada capa sólo enlaza con las capas internas:
#   presentation -> infrastructure -> application -> domain
# ---------------------------------------------------------------------------
add_library(testores_domain STATIC
    src/domain/column_set.cpp
    src/domain/boolean_matrix.cpp
    src/domain/basic_matrix.cpp
    src/domain/testor.cpp
    src/domain/yyc.cpp
    src/domain/notation.cpp)
target_include_directories(testores_domain PUBLIC include)
target_link_libraries(testores_domain PRIVATE testores_options)

add_library(testores_application STATIC
    src/application/matrix_generation.cpp
    src/application/yyc_runner.cpp)
target_link_libraries(testores_application PUBLIC testores_domain PRIVATE testores_options)

add_library(testores_infrastructure STATIC
    src/infrastructure/system_adapters.cpp
    src/infrastructure/output_file.cpp
    src/infrastructure/matrix_file.cpp
    src/infrastructure/result_export.cpp)
target_link_libraries(testores_infrastructure PUBLIC testores_application PRIVATE testores_options)

add_library(testores_presentation STATIC
    src/presentation/console_input.cpp
    src/presentation/console_view.cpp
    src/presentation/cli.cpp)
target_link_libraries(testores_presentation PUBLIC testores_infrastructure PRIVATE testores_options)

add_executable(testores apps/main.cpp)
target_link_libraries(testores PRIVATE testores_presentation testores_options)
if(MINGW)
    # Ejecutable autocontenido: no depende de las DLL de MinGW en el PATH.
    target_link_options(testores PRIVATE -static)
endif()

# ---------------------------------------------------------------------------
# Pruebas
# ---------------------------------------------------------------------------
if(TESTORES_BUILD_TESTS)
    enable_testing()
    add_executable(testores_tests
        tests/test_main.cpp
        tests/test_domain.cpp
        tests/test_yyc.cpp
        tests/test_application.cpp
        tests/test_infrastructure.cpp
        tests/test_presentation.cpp)
    target_include_directories(testores_tests PRIVATE tests)
    target_link_libraries(testores_tests PRIVATE testores_presentation testores_options)
    if(MINGW)
        target_link_options(testores_tests PRIVATE -static)
    endif()
    add_test(NAME testores_tests COMMAND testores_tests)
endif()
