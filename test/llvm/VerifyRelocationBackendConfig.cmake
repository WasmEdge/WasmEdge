cmake_minimum_required(VERSION 3.18)

foreach(Argument IN ITEMS WASMEDGE_SOURCE_DIR BUILD_DIR EXPECTED_BACKENDS)
  if(NOT DEFINED ${Argument})
    message(FATAL_ERROR "${Argument} is required")
  endif()
endforeach()

include("${WASMEDGE_SOURCE_DIR}/cmake/LLVMRelocationBackends.cmake")
include("${WASMEDGE_SOURCE_DIR}/test/llvm/CMakeTestHelpers.cmake")

set(CacheFile "${BUILD_DIR}/CMakeCache.txt")
if(NOT EXISTS "${CacheFile}")
  message(FATAL_ERROR "CMake cache is missing: ${CacheFile}")
endif()
wasmedge_read_cmake_cache_value(ActualBackends "${CacheFile}"
  WASMEDGE_LLVM_LINKER_RELOCATION_BACKENDS_EFFECTIVE INTERNAL)
if(NOT ActualBackends STREQUAL EXPECTED_BACKENDS)
  message(FATAL_ERROR
    "Effective relocation backends are '${ActualBackends}', expected '${EXPECTED_BACKENDS}'")
endif()

set(ManifestFile "${BUILD_DIR}/lib/llvm/relocation_backends.cmake")
if(NOT EXISTS "${ManifestFile}")
  message(FATAL_ERROR "Relocation backend manifest is missing: ${ManifestFile}")
endif()
include("${ManifestFile}")

set(ExpectedProperties "")
foreach(Backend IN LISTS WASMEDGE_LLVM_LINKER_ALL_RELOCATION_BACKENDS)
  wasmedge_llvm_linker_relocation_source(Source ${Backend})

  if(Backend IN_LIST EXPECTED_BACKENDS)
    set(ExpectedPresent TRUE)
    list(APPEND ExpectedProperties 1)
  else()
    set(ExpectedPresent FALSE)
    list(APPEND ExpectedProperties 0)
  endif()
  if(Source IN_LIST WASMEDGE_LINKER_TARGET_SOURCES)
    set(ActualPresent TRUE)
  else()
    set(ActualPresent FALSE)
  endif()
  if(NOT ActualPresent STREQUAL ExpectedPresent)
    message(FATAL_ERROR
      "Relocation source '${Source}' presence is ${ActualPresent}, expected ${ExpectedPresent} for ${Backend}")
  endif()
endforeach()

if(NOT WASMEDGE_LINKER_RELOCATION_PROPERTIES STREQUAL ExpectedProperties)
  message(FATAL_ERROR
    "Relocation properties are '${WASMEDGE_LINKER_RELOCATION_PROPERTIES}', expected '${ExpectedProperties}' in X86_64/AARCH64/ARM/RISCV64/S390X order")
endif()
