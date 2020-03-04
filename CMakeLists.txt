# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.9)
project(SSVM)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

find_package(Boost REQUIRED)
find_package(ONNC-wasm)

if(ONNC_WASM_LIBRARY)
  add_definitions(-DONNC_WASM)
endif()

include_directories(BEFORE
  ${CMAKE_CURRENT_BINARY_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_BINARY_DIR}/thirdparty
  ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
  ${CMAKE_CURRENT_BINARY_DIR}/thirdparty/evmc/include
  ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/evmc/include
  ${Boost_INCLUDE_DIR}
)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BUILD_TESTS "Generate build targets for the ssvm unit tests." OFF)

# Macro for copying directory.
macro(configure_files srcDir destDir)
message(STATUS "Generating testing input directory ${destDir}")
make_directory(${destDir})

file(GLOB templateFiles RELATIVE ${srcDir} ${srcDir}/*)
foreach(templateFile ${templateFiles})
    set(srcTemplatePath ${srcDir}/${templateFile})
    if(NOT IS_DIRECTORY ${srcTemplatePath})
        configure_file(
                ${srcTemplatePath}
                ${destDir}/${templateFile}
                COPYONLY)
    else()
        configure_files(
                ${srcTemplatePath}
                ${destDir}/${templateFile}
                COPYONLY)
    endif()
endforeach(templateFile)
endmacro(configure_files)
# End of Macro

if(BUILD_TESTS)
  include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}/thirdparty/googletest/include
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/googletest/include
    ${CMAKE_CURRENT_BINARY_DIR}/thirdparty/googletest/
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/googletest/
  )
  add_subdirectory(test)
endif()

add_subdirectory(include)
add_subdirectory(lib)
add_subdirectory(thirdparty)
add_subdirectory(tools)
