cmake_minimum_required(VERSION 3.18)

include("${WASMEDGE_SOURCE_DIR}/test/llvm/CMakeTestHelpers.cmake")

file(READ "${WASMEDGE_SOURCE_DIR}/test/llvm/CMakeLists.txt" LLVMTestCMake)
if(LLVMTestCMake MATCHES "\\$<LINK_LIBRARY:")
  message(FATAL_ERROR
    "test/llvm/CMakeLists.txt must not use $<LINK_LIBRARY:> (requires CMake 3.24)")
endif()

set(Value "path with space;segment\\;leaf")
wasmedge_cmake_cache_argument(Argument ROUNDTRIP STRING "${Value}")
set(Command "${CMAKE_COMMAND}")
list(APPEND Command "${Argument}")
execute_process(
  COMMAND ${Command}
    -P "${WASMEDGE_SOURCE_DIR}/test/llvm/CMakeArgumentChild.cmake"
  RESULT_VARIABLE Result
  OUTPUT_VARIABLE Output
  ERROR_VARIABLE Error
)
if(NOT Result EQUAL 0)
  message(FATAL_ERROR "cache argument round trip failed: ${Output}${Error}")
endif()

set(CacheFile "${CMAKE_CURRENT_BINARY_DIR}/cmake-cache-value-test.txt")
file(WRITE "${CacheFile}"
  "ARCHITECTURES:STRING=x86_64\\;arm64\nOTHER:STRING=value\n")
wasmedge_read_cmake_cache_value(
  CacheValue "${CacheFile}" ARCHITECTURES STRING
)
file(REMOVE "${CacheFile}")
if(NOT CacheValue STREQUAL "x86_64;arm64")
  message(FATAL_ERROR "cache list value was '${CacheValue}'")
endif()
