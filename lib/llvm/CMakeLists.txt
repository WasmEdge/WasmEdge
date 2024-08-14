# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

find_package(LLVM REQUIRED HINTS "${LLVM_CMAKE_PATH}")
get_filename_component(LLVM_DIR "${LLVM_DIR}" ABSOLUTE)
list(APPEND CMAKE_MODULE_PATH ${LLVM_DIR})
include(LLVMConfig)
include(AddLLVM)

get_filename_component(LLD_DIR "${LLVM_LIBRARY_DIR}/cmake/lld" ABSOLUTE)
find_package(LLD REQUIRED HINTS ${LLD_DIR})

if(WASMEDGE_LINK_LLVM_STATIC)
  wasmedge_add_library(wasmedgeLLVM
    compiler.cpp
    codegen.cpp
    data.cpp
    jit.cpp
  )

  target_link_libraries(wasmedgeLLVM
    PUBLIC
    wasmedgeCommon
    wasmedgeSystem
    std::filesystem
    ${WASMEDGE_LLVM_LINK_STATIC_COMPONENTS}
    ${WASMEDGE_LLVM_LINK_SHARED_COMPONENTS}
  )
else()
  if(APPLE)
    list(APPEND LLD_LIBS lldMachO)
  elseif(WIN32)
    list(APPEND LLD_LIBS lldCOFF)
    set(EXTRA_COMPONENTS DebugInfoPDB LibDriver WindowsManifest)
  else()
    list(APPEND LLD_LIBS lldELF)
  endif()

  list(APPEND LLD_LIBS lldCommon)
  if(LLVM_VERSION_MAJOR LESS_EQUAL 13)
    list(APPEND LLD_LIBS lldDriver)
  endif()

  # Command line warning D9025 : overriding '/EHs' with '/EHs-'
  # prevent LLVM from adding /EHs-c-.
  if(WIN32)
    set(LLVM_REQUIRES_EH ON)
  endif()

  llvm_add_library(wasmedgeLLVM
    compiler.cpp
    codegen.cpp
    data.cpp
    jit.cpp
    LINK_LIBS
    wasmedgeCommon
    wasmedgeSystem
    ${LLD_LIBS}
    std::filesystem
    ${CMAKE_THREAD_LIBS_INIT}
    LINK_COMPONENTS
    core
    lto
    native
    nativecodegen
    option
    passes
    support
    orcjit
    transformutils
    ${EXTRA_COMPONENTS}
  )
endif()

wasmedge_setup_target(wasmedgeLLVM)

target_include_directories(wasmedgeLLVM
  SYSTEM
  PRIVATE
  ${LLVM_INCLUDE_DIR}
)

target_include_directories(wasmedgeLLVM
  PUBLIC
  ${PROJECT_BINARY_DIR}/include
)

include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_LINK_OPTIONS "-Wl,-lc,--exclude-libs=libc.a")
check_cxx_source_compiles("int main(){}" SUPPORT_EXCLUDE_LIBS)

if(SUPPORT_EXCLUDE_LIBS)
  file(GLOB LLD_ALL_LIBS ${LLVM_LIBRARY_DIR}/liblld*.a)
  list(TRANSFORM LLD_ALL_LIBS REPLACE "^.*/" "")
  string(JOIN : LLD_ALL_LIBS_COLON ${LLD_ALL_LIBS})
  if(NOT LLD_ALL_LIBS_COLON STREQUAL "")
    target_link_options(wasmedgeLLVM
      PUBLIC
      "SHELL:-Wl,--exclude-libs=${LLD_ALL_LIBS_COLON}"
    )
  endif()
endif()
