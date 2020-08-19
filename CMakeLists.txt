# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.11)
project(SSVM)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()
option(STATIC_BUILD "Link glibcxx library statically." OFF)

find_package(Boost REQUIRED)
find_package(Filesystem REQUIRED Final Experimental)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BUILD_TESTS "Generate build targets for the ssvm unit tests." OFF)
option(BUILD_COVERAGE "Generate coverage report. Require BUILD_TESTS." OFF)

if(BUILD_COVERAGE)
  include(CodeCoverage)
  append_coverage_compiler_flags()
endif()

# List of SSVM runtimes
option(SSVM_DISABLE_AOT_RUNTIME "Disable SSVM LLVM-based ahead of time compilation runtime." OFF)

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
  include(CTest)
  add_subdirectory(test)
endif()

add_subdirectory(include)
add_subdirectory(lib)
add_subdirectory(thirdparty)
add_subdirectory(tools)
