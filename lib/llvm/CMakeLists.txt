# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

find_package(LLVM REQUIRED HINTS "${LLVM_CMAKE_PATH}" "${LLVM_DIR}")
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(LLVMConfig)
include(AddLLVM)

set(LLVM_INCLUDE_DIR "${LLVM_INCLUDE_DIR}" PARENT_SCOPE)

set(WASMEDGE_LLVM_SOURCES
  arm_runtime_libcalls.cpp
  compiler.cpp
  compiler/context.cpp
  compiler/function_compiler.cpp
  compiler/memoryInstr.cpp
  compiler/numericInstr.cpp
  compiler/refInstr.cpp
  compiler/threadInstr.cpp
  compiler/vectorInstr.cpp
  codegen.cpp
  data.cpp
  jit.cpp
  lazyjit.cpp
  llvm.cpp
)

set(WASMEDGE_LLVM_LINKER_SOURCES
  linker/elf_writer.cpp
  linker/compact_unwind.cpp
  linker/eh_frame.cpp
  linker/layout.cpp
  linker/link_graph.cpp
  linker/macho_writer.cpp
  linker/native_linker.cpp
  linker/pe_writer.cpp
  linker/object_reader.cpp
  linker/relocation.cpp
  linker/universal_wasm_writer.cpp
  linker/writer.cpp
)

set(WASMEDGE_LINKER_HAS_X86_64 0)
set(WASMEDGE_LINKER_HAS_AARCH64 0)
set(WASMEDGE_LINKER_HAS_ARM 0)
set(WASMEDGE_LINKER_HAS_RISCV64 0)
set(WASMEDGE_LINKER_HAS_S390X 0)
foreach(BACKEND IN LISTS WASMEDGE_LLVM_LINKER_RELOCATION_BACKENDS_EFFECTIVE)
  if(BACKEND STREQUAL "X86_64")
    list(APPEND WASMEDGE_LLVM_LINKER_SOURCES linker/relocation_x86_64.cpp)
    set(WASMEDGE_LINKER_HAS_X86_64 1)
  elseif(BACKEND STREQUAL "AARCH64")
    list(APPEND WASMEDGE_LLVM_LINKER_SOURCES linker/relocation_aarch64.cpp)
    set(WASMEDGE_LINKER_HAS_AARCH64 1)
  elseif(BACKEND STREQUAL "ARM")
    list(APPEND WASMEDGE_LLVM_LINKER_SOURCES linker/relocation_arm.cpp)
    set(WASMEDGE_LINKER_HAS_ARM 1)
  elseif(BACKEND STREQUAL "RISCV64")
    list(APPEND WASMEDGE_LLVM_LINKER_SOURCES linker/relocation_riscv.cpp)
    set(WASMEDGE_LINKER_HAS_RISCV64 1)
  elseif(BACKEND STREQUAL "S390X")
    list(APPEND WASMEDGE_LLVM_LINKER_SOURCES linker/relocation_s390x.cpp)
    set(WASMEDGE_LINKER_HAS_S390X 1)
  endif()
endforeach()
list(APPEND WASMEDGE_LLVM_SOURCES ${WASMEDGE_LLVM_LINKER_SOURCES})

if(MSVC)
  set_source_files_properties(${WASMEDGE_LLVM_LINKER_SOURCES}
    PROPERTIES COMPILE_OPTIONS /EHsc)
else()
  set_source_files_properties(${WASMEDGE_LLVM_LINKER_SOURCES}
    PROPERTIES COMPILE_OPTIONS -fexceptions)
endif()

if(WASMEDGE_LINK_LLVM_STATIC)
  wasmedge_add_library(wasmedgeLLVM
    ${WASMEDGE_LLVM_SOURCES}
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
  if(WIN32)
    set(EXTRA_COMPONENTS DebugInfoPDB LibDriver WindowsManifest)
  endif()

  # Command line warning D9025 : overriding '/EHs' with '/EHs-'
  # prevent LLVM from adding /EHs-c-.
  if(WIN32)
    set(LLVM_REQUIRES_EH ON)
  endif()

  llvm_add_library(wasmedgeLLVM
    ${WASMEDGE_LLVM_SOURCES}
    LINK_LIBS
    wasmedgeCommon
    wasmedgeSystem
    std::filesystem
    ${CMAKE_THREAD_LIBS_INIT}
    LINK_COMPONENTS
    binaryformat
    core
    native
    nativecodegen
    object
    passes
    support
    orcjit
    transformutils
    ${EXTRA_COMPONENTS}
  )
endif()

wasmedge_setup_target(wasmedgeLLVM)

target_compile_definitions(wasmedgeLLVM PRIVATE
  WASMEDGE_LINKER_HAS_X86_64=${WASMEDGE_LINKER_HAS_X86_64}
  WASMEDGE_LINKER_HAS_AARCH64=${WASMEDGE_LINKER_HAS_AARCH64}
  WASMEDGE_LINKER_HAS_ARM=${WASMEDGE_LINKER_HAS_ARM}
  WASMEDGE_LINKER_HAS_RISCV64=${WASMEDGE_LINKER_HAS_RISCV64}
  WASMEDGE_LINKER_HAS_S390X=${WASMEDGE_LINKER_HAS_S390X}
)
set_property(TARGET wasmedgeLLVM PROPERTY
  WASMEDGE_LINKER_HAS_X86_64
  ${WASMEDGE_LINKER_HAS_X86_64}
)
set_property(TARGET wasmedgeLLVM PROPERTY
  WASMEDGE_LINKER_HAS_AARCH64
  ${WASMEDGE_LINKER_HAS_AARCH64}
)
set_property(TARGET wasmedgeLLVM PROPERTY
  WASMEDGE_LINKER_HAS_ARM
  ${WASMEDGE_LINKER_HAS_ARM}
)
set_property(TARGET wasmedgeLLVM PROPERTY
  WASMEDGE_LINKER_HAS_RISCV64
  ${WASMEDGE_LINKER_HAS_RISCV64}
)
set_property(TARGET wasmedgeLLVM PROPERTY
  WASMEDGE_LINKER_HAS_S390X
  ${WASMEDGE_LINKER_HAS_S390X}
)

file(GENERATE
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/relocation_backends.cmake
  CONTENT
"set(WASMEDGE_LINKER_TARGET_SOURCES \"$<JOIN:$<TARGET_PROPERTY:wasmedgeLLVM,SOURCES>,;>\")
set(WASMEDGE_LINKER_RELOCATION_DEFINITIONS \"$<JOIN:$<TARGET_PROPERTY:wasmedgeLLVM,COMPILE_DEFINITIONS>,;>\")
set(WASMEDGE_LINKER_RELOCATION_PROPERTIES \"$<TARGET_PROPERTY:wasmedgeLLVM,WASMEDGE_LINKER_HAS_X86_64>;$<TARGET_PROPERTY:wasmedgeLLVM,WASMEDGE_LINKER_HAS_AARCH64>;$<TARGET_PROPERTY:wasmedgeLLVM,WASMEDGE_LINKER_HAS_ARM>;$<TARGET_PROPERTY:wasmedgeLLVM,WASMEDGE_LINKER_HAS_RISCV64>;$<TARGET_PROPERTY:wasmedgeLLVM,WASMEDGE_LINKER_HAS_S390X>\")
")

target_include_directories(wasmedgeLLVM
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(wasmedgeLLVM
  SYSTEM
  PRIVATE
  ${LLVM_INCLUDE_DIR}
)

target_include_directories(wasmedgeLLVM
  PUBLIC
  ${PROJECT_BINARY_DIR}/include
)
