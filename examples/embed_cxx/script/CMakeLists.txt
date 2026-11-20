cmake_minimum_required(VERSION 3.16)

project(script)

set(CMAKE_CXX_STANDARD 20)

if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
    add_executable(
        fibonacci
        fibonacci.cpp
    )
    set_target_properties(fibonacci PROPERTIES SUFFIX ".wasm")

    target_link_options(
        fibonacci
        PRIVATE
        -mexec-model=reactor
    )

    add_custom_command(
        TARGET fibonacci
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:fibonacci>" "${WASM_OUT_DIR}" VERBATIM
    )
else()
    add_library(
        fibonacci
        SHARED
        fibonacci.cpp
    )
endif()
