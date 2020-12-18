# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.9.4)
project(SSVM)

find_program(GIT_CMD git)
execute_process(COMMAND ${GIT_CMD} describe --match "*[0-9].[0-9]*" --tag OUTPUT_VARIABLE CPACK_PACKAGE_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()
option(STATIC_BUILD "Link glibcxx library statically." OFF)

find_package(Boost REQUIRED)
find_package(Filesystem REQUIRED Final Experimental)

if(CMAKE_BUILD_TYPE STREQUAL Release OR CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_ENABLE_EXPORTS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
set(CMAKE_SKIP_RPATH ON)

option(BUILD_TESTS "Generate build targets for the ssvm unit tests." OFF)
option(BUILD_COVERAGE "Generate coverage report. Require BUILD_TESTS." OFF)
option(BUILD_TOOLS "Generate ssvm and ssvmc tools." ON)
set(BUILD_PACKAGE "DEB;RPM" CACHE STRING "Package generate types")

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

set(CPACK_PACKAGE_VENDOR Second State LLC)
set(CPACK_STRIP_FILES ON)
set(CPACK_PACKAGE_CONTACT "Shen-Ta Hsieh <beststeve@secondstate.io>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "High performance WebAssembly Virtual Machine")
set(CPACK_DEBIAN_COMPRESSION_TYPE xz)
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/second-state/SSVM/")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEP ON)
set(CPACK_DEBIAN_PACKAGE_DEPENDS libllvm10)
set(CPACK_GENERATOR "${BUILD_PACKAGE}")
set(CPACK_PACKAGE_DESCRIPTION "SSVM is a high performance, extensible, and hardware optimized WebAssembly Virtual Machine for cloud, AI, and blockchain applications.")

include(CPack)

if(BUILD_TESTS)
  include(CTest)
  add_subdirectory(test)
endif()

add_subdirectory(include)
add_subdirectory(lib)
add_subdirectory(thirdparty)
if(BUILD_TOOLS)
  add_subdirectory(tools)
endif()
