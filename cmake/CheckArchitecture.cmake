cmake_minimum_required(VERSION 3.16)

get_filename_component(TESTORES_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(TESTORES_LAYERS domain application infrastructure presentation)

foreach(layer IN LISTS TESTORES_LAYERS)
    if(layer STREQUAL "domain")
        set(allowed domain)
    elseif(layer STREQUAL "application")
        set(allowed domain application)
    elseif(layer STREQUAL "infrastructure")
        set(allowed domain application infrastructure)
    else()
        set(allowed domain application presentation)
    endif()

    file(GLOB_RECURSE layer_files
        "${TESTORES_ROOT}/include/testores/${layer}/*.hpp"
        "${TESTORES_ROOT}/src/${layer}/*.hpp"
        "${TESTORES_ROOT}/src/${layer}/*.cpp")
    foreach(source IN LISTS layer_files)
        file(STRINGS "${source}" lines)
        set(line_number 0)
        foreach(line IN LISTS lines)
            math(EXPR line_number "${line_number} + 1")
            if(line MATCHES "^[ \t]*#[ \t]*include[ \t]*[<\"]([^>\"]+)[>\"]")
                set(included "${CMAKE_MATCH_1}")
                # Las dependencias públicas usan testores/<capa>/; evitar que
                # rutas relativas oculten dependencias entre capas.
                if(included MATCHES "(^|/)\\.\\.(/|$)")
                    message(FATAL_ERROR "${source}:${line_number}: include relativo fuera de la capa: ${included}")
                endif()
                if(included MATCHES "^testores/([^/]+)/")
                    set(dependency "${CMAKE_MATCH_1}")
                    if(NOT dependency IN_LIST allowed)
                        message(FATAL_ERROR "${source}:${line_number}: ${layer} no puede depender de ${dependency}")
                    endif()
                endif()
            endif()
        endforeach()
    endforeach()
endforeach()

message(STATUS "Arquitectura verificada: dominio y aplicación internos; adaptadores y presentación independientes")
