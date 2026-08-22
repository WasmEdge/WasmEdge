# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

find_package(LLVM REQUIRED HINTS "${LLVM_CMAKE_PATH}" "${LLVM_DIR}")
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(LLVMConfig)
include(AddLLVM)

if(WASMEDGE_USE_LLD)
  find_package(LLD REQUIRED HINTS "${LLD_CMAKE_PATH}" "${LLD_DIR}" "${LLVM_LIBRARY_DIR}/cmake/lld")
  list(APPEND CMAKE_MODULE_PATH "${LLD_CMAKE_DIR}")
endif()

set(LLVM_INCLUDE_DIR "${LLVM_INCLUDE_DIR}" PARENT_SCOPE)

set(WASMEDGE_LLVM_SOURCES
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
  linker/byte_io.cpp
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

foreach(BACKEND IN LISTS WASMEDGE_LLVM_LINKER_ALL_RELOCATION_BACKENDS)
  set(WASMEDGE_LINKER_HAS_${BACKEND} 0)
endforeach()
foreach(BACKEND IN LISTS WASMEDGE_LLVM_LINKER_RELOCATION_BACKENDS_EFFECTIVE)
  wasmedge_llvm_linker_relocation_source(SOURCE ${BACKEND})
  list(APPEND WASMEDGE_LLVM_LINKER_SOURCES ${SOURCE})
  set(WASMEDGE_LINKER_HAS_${BACKEND} 1)
endforeach()
list(APPEND WASMEDGE_LLVM_SOURCES ${WASMEDGE_LLVM_LINKER_SOURCES})
if(WASMEDGE_USE_LLD)
  list(APPEND WASMEDGE_LLVM_SOURCES lld.cpp)
else()
  set(LLVM_OPTIONAL_SOURCES lld.cpp)
endif()

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

  if(NOT LLVM_ENABLE_RTTI)
    if(MSVC)
      target_compile_options(wasmedgeLLVM PRIVATE /GR-)
    else()
      target_compile_options(wasmedgeLLVM PRIVATE -fno-rtti)
    endif()
  endif()

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

  if(WASMEDGE_USE_LLD)
    if(APPLE)
      list(APPEND LLD_LIBS lldMachO)
    elseif(WIN32)
      list(APPEND LLD_LIBS lldCOFF)
    else()
      list(APPEND LLD_LIBS lldELF)
    endif()
    list(APPEND LLD_LIBS lldCommon)
    if(LLVM_VERSION_MAJOR LESS_EQUAL 13)
      list(APPEND LLD_LIBS lldDriver)
    endif()
    list(APPEND EXTRA_COMPONENTS linker lto option)
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
    ${LLD_LIBS}
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

if(WASMEDGE_USE_LLD)
  target_compile_definitions(wasmedgeLLVM PUBLIC WASMEDGE_USE_LLD)
endif()

set(WASMEDGE_LINKER_RELOCATION_PROPERTIES "")
foreach(BACKEND IN LISTS WASMEDGE_LLVM_LINKER_ALL_RELOCATION_BACKENDS)
  target_compile_definitions(wasmedgeLLVM PRIVATE
    WASMEDGE_LINKER_HAS_${BACKEND}=${WASMEDGE_LINKER_HAS_${BACKEND}}
  )
  set_property(TARGET wasmedgeLLVM PROPERTY
    WASMEDGE_LINKER_HAS_${BACKEND}
    ${WASMEDGE_LINKER_HAS_${BACKEND}}
  )
  list(APPEND WASMEDGE_LINKER_RELOCATION_PROPERTIES
    "$<TARGET_PROPERTY:wasmedgeLLVM,WASMEDGE_LINKER_HAS_${BACKEND}>")
endforeach()

file(GENERATE
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/relocation_backends.cmake
  CONTENT
"set(WASMEDGE_LINKER_TARGET_SOURCES \"$<JOIN:$<TARGET_PROPERTY:wasmedgeLLVM,SOURCES>,;>\")
set(WASMEDGE_LINKER_RELOCATION_PROPERTIES \"${WASMEDGE_LINKER_RELOCATION_PROPERTIES}\")
")

target_include_directories(wasmedgeLLVM
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(wasmedgeLLVM
  SYSTEM
  PRIVATE
  ${LLVM_INCLUDE_DIR}
  ${LLD_INCLUDE_DIRS}
)

target_include_directories(wasmedgeLLVM
  PUBLIC
  ${PROJECT_BINARY_DIR}/include
)

if(WASMEDGE_USE_LLD)
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
endif()
