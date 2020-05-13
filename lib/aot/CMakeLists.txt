# SPDX-License-Identifier: Apache-2.0

find_package(Threads)
find_package(LLVM REQUIRED HINTS "${LLVM_CMAKE_PATH}")
list(APPEND CMAKE_MODULE_PATH ${LLVM_DIR})
include(AddLLVM)

find_library(LLD_ELF lldELF PATHS "${LLVM_LIBRARY_DIR}")
find_library(LLD_COMMON lldCommon PATHS "${LLVM_LIBRARY_DIR}")

add_llvm_library(ssvmAOT
  compiler.cpp
  LINK_LIBS
  ${LLD_ELF}
  ${LLD_COMMON}
  ${CMAKE_THREAD_LIBS_INIT}
  LINK_COMPONENTS
  core
  native
  nativecodegen
  passes
  transformutils
  support
)

target_include_directories(ssvmAOT
  SYSTEM
  PRIVATE
  "${LLVM_INCLUDE_DIR}"
)
target_include_directories(ssvmAOT
  PUBLIC
  ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(ssvmAOT
  PRIVATE
  std::filesystem
)
