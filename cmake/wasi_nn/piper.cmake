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
        NO_DEFAULT_PATH REQUIRED)
      set(CMAKE_FIND_LIBRARY_SUFFIXES ${_OLD_SUFFIXES})
      set_target_properties(
        piper PROPERTIES IMPORTED_LOCATION "${PIPER_LIB_PATH}"
                         INTERFACE_INCLUDE_DIRECTORIES "${PIPER_ROOT}/include")
    else()
      message(STATUS "Downloading piper source")
      find_program(GIT_CMD git REQUIRED)
      FetchContent_Declare(
        piper
        GIT_REPOSITORY https://github.com/OHF-Voice/piper1-gpl.git
        GIT_TAG 32b95f8c1f0dc0ce27a6acd1143de331f61af777
        UPDATE_DISCONNECTED TRUE
        SOURCE_SUBDIR libpiper
        PATCH_COMMAND "${GIT_CMD}" "apply" "${CMAKE_SOURCE_DIR}/utils/wasi-nn/libpiper.patch"
      )
      set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "Piper not build shared")
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
