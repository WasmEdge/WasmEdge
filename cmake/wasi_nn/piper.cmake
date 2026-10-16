# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

include_guard()

include(FetchContent)

# Function for preparing the piper dependency.
function(wasmedge_setup_piper_target target)
  if(NOT TARGET piper)
    # setup piper
    if(DEFINED PIPER_ROOT)
      message(STATUS "Build: Using pre-built Piper from ${PIPER_ROOT}")
      add_library(piper STATIC IMPORTED)
      set(_OLD_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
      set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
      find_library(
        PIPER_LIB_PATH
        NAMES piper libpiper
        PATHS "${PIPER_ROOT}"
        PATH_SUFFIXES lib
        NO_DEFAULT_PATH REQUIRED)
      set(CMAKE_FIND_LIBRARY_SUFFIXES ${_OLD_SUFFIXES})
      set_target_properties(
        piper PROPERTIES IMPORTED_LOCATION "${PIPER_LIB_PATH}"
                         INTERFACE_INCLUDE_DIRECTORIES "${PIPER_ROOT}/include")
      find_library(ONNXRUNTIME_LIB
        NAMES onnxruntime libonnxruntime
        PATHS "${PIPER_ROOT}/lib" "${PIPER_ROOT}/lib64"
        NO_DEFAULT_PATH)
      if(NOT ONNXRUNTIME_LIB)
        find_library(ONNXRUNTIME_LIB NAMES onnxruntime libonnxruntime REQUIRED)
      endif()
      find_library(ESPEAK_NG_LIB
        NAMES espeak-ng libespeak-ng
        PATHS "${PIPER_ROOT}/lib" "${PIPER_ROOT}/lib64"
        NO_DEFAULT_PATH)
      if(NOT ESPEAK_NG_LIB)
        find_library(ESPEAK_NG_LIB NAMES espeak-ng libespeak-ng REQUIRED)
      endif()
      find_library(UCD_LIB
        NAMES ucd libucd
        PATHS "${PIPER_ROOT}/lib" "${PIPER_ROOT}/lib64"
        NO_DEFAULT_PATH)
      if(NOT UCD_LIB)
        find_library(UCD_LIB NAMES ucd libucd REQUIRED)
      endif()
      set_target_properties(
        piper PROPERTIES INTERFACE_LINK_LIBRARIES
                         "${ONNXRUNTIME_LIB};${ESPEAK_NG_LIB};${UCD_LIB}")
    else()
      message(STATUS "Downloading piper source")
      find_program(GIT_CMD git REQUIRED)
      FetchContent_Declare(
        piper
        GIT_REPOSITORY https://github.com/OHF-Voice/piper1-gpl.git
        GIT_TAG 639388b6317fc4731e91d53da42aea68fd4166ff
        UPDATE_DISCONNECTED TRUE
        SOURCE_SUBDIR libpiper
        PATCH_COMMAND "${GIT_CMD}" "apply" "${CMAKE_SOURCE_DIR}/utils/wasi-nn/libpiper.patch"
      )
      set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "Piper not build shared")
      set(PIPER_BUILD_EXE OFF CACHE INTERNAL "Piper not build the command line tool")
      set(BUILD_TESTING OFF CACHE INTERNAL "Piper not build tests")
      set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE INTERNAL "Piper build independent code")
      FetchContent_MakeAvailable(piper)
      message(STATUS "Downloading piper source -- done")
      # suppress src/cpp/piper.cpp:302:29: error: unused parameter ‘config’ [-Werror=unused-parameter]
      target_compile_options(piper PRIVATE -Wno-error=unused-parameter)
    endif()
  endif()
  wasmedge_setup_simdjson()
  target_link_libraries(${target}
    PRIVATE
    piper
    simdjson::simdjson
  )
endfunction()
