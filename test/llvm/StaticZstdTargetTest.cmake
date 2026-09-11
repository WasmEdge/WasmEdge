set(Source "${CMAKE_CURRENT_BINARY_DIR}/static-zstd-target-source")
set(Build "${CMAKE_CURRENT_BINARY_DIR}/static-zstd-target-build")
file(REMOVE_RECURSE "${Source}" "${Build}")
file(MAKE_DIRECTORY "${Source}")
file(WRITE "${Source}/CMakeLists.txt"
"cmake_minimum_required(VERSION 3.18)
project(StaticZstdTarget NONE)
list(APPEND CMAKE_MODULE_PATH \"${WASMEDGE_SOURCE_DIR}/cmake\")
include(StaticZstd)
add_library(zstd::libzstd_static STATIC IMPORTED)
wasmedge_select_static_zstd_archive(Archive)
if(NOT Archive STREQUAL \"zstd::libzstd_static\")
  message(FATAL_ERROR \"static zstd target selected '\${Archive}'\")
endif()
")
execute_process(
  COMMAND "${CMAKE_COMMAND}" -S "${Source}" -B "${Build}"
  RESULT_VARIABLE Result
  OUTPUT_VARIABLE Output
  ERROR_VARIABLE Error
)
file(REMOVE_RECURSE "${Source}" "${Build}")
if(NOT Result EQUAL 0)
  message(FATAL_ERROR "static zstd target configure failed: ${Output}${Error}")
endif()
