# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

set(WASMEDGE_INTERPROCEDURAL_OPTIMIZATION OFF)
if(CMAKE_BUILD_TYPE STREQUAL Release OR CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo)
  if(NOT WASMEDGE_FORCE_DISABLE_LTO)
    set(WASMEDGE_INTERPROCEDURAL_OPTIMIZATION ON)
  endif()
  if(CMAKE_GENERATOR STREQUAL Ninja)
    if(CMAKE_COMPILER_IS_GNUCXX)
      list(TRANSFORM CMAKE_C_COMPILE_OPTIONS_IPO REPLACE "^-flto$" "-flto=auto")
      list(TRANSFORM CMAKE_CXX_COMPILE_OPTIONS_IPO REPLACE "^-flto$" "-flto=auto")
    endif()
    set(CMAKE_JOB_POOLS "link=2")
    set(CMAKE_JOB_POOL_LINK link)
  endif()
endif()
if(NOT WASMEDGE_USE_CXX11_ABI)
  add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  list(APPEND WASMEDGE_CFLAGS
    /utf-8
    /WX
    /W4
    /we5030 # treat unknown attribute as error
    # disable some warnings
    /wd4201 # nonstandard extension used: nameless struct/union
    /wd4141 # 'inline': used more than once
    /wd4324 # structure was padded due to alignment specifier
    /wd4702 # unreachable code
    /wd4819 # file contains a character not in current code page
    /wd4127 # conditional expression is constant
    /wd4611 # interaction between '_setjmp' and C++ object destruction is non-portable
  )
else()
  list(APPEND WASMEDGE_CFLAGS
    -Wall
    -Wextra
  )

  if(NOT WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_CUBLAS)
    list(APPEND WASMEDGE_CFLAGS
      -Werror
      -Wno-error=pedantic
    )
  endif()

  if(WASMEDGE_ENABLE_UB_SANITIZER)
    list(APPEND WASMEDGE_CFLAGS -fsanitize=undefined)
  endif()

  if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    list(APPEND WASMEDGE_CFLAGS -Wno-psabi)
  endif()
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  list(APPEND WASMEDGE_CFLAGS -Wno-c++20-designator)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  list(APPEND WASMEDGE_CFLAGS
    -Wno-c99-extensions
    -Wno-covered-switch-default
    -Wno-documentation-unknown-command
    -Wno-error=nested-anon-types
    -Wno-error=old-style-cast
    -Wno-error=shadow
    -Wno-error=unused-command-line-argument
    -Wno-error=unknown-warning-option
    -Wno-ctad-maybe-unsupported
    -Wno-gnu-anonymous-struct
    -Wno-keyword-macro
    -Wno-language-extension-token
    -Wno-newline-eof
    -Wno-shadow-field-in-constructor
    -Wno-signed-enum-bitfield
    -Wno-switch-enum
    -Wno-undefined-func-template
  )

  if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 17.0.0)
    list(APPEND WASMEDGE_CFLAGS
      -Wno-deprecated-literal-operator
    )
  endif()

  if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 18.0.0)
    list(APPEND WASMEDGE_CFLAGS
      -Wno-switch-default
    )
  endif()

  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 13.0.0)
    list(APPEND WASMEDGE_CFLAGS
      -Wno-error=return-std-move-in-c++11
    )
  elseif(NOT CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    list(APPEND WASMEDGE_CFLAGS
      -Wno-error=shadow-field
      -Wno-reserved-identifier
    )
  endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 13)
    list(APPEND WASMEDGE_CFLAGS
      -Wno-error=dangling-reference
    )
  endif()
  if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 14)
    list(APPEND WASMEDGE_CFLAGS
      -Wno-error=template-id-cdtor
    )
  endif()
endif()

if(WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_ENABLE_EXTENDED_ALIGNED_STORAGE -DNOMINMAX -D_ITERATOR_DEBUG_LEVEL=0)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    list(APPEND WASMEDGE_CFLAGS
      -Wno-c++98-compat
      -Wno-c++98-compat-pedantic
      -Wno-exit-time-destructors
      -Wno-global-constructors
      -Wno-used-but-marked-unused
      -Wno-nonportable-system-include-path
      -Wno-float-equal
      -Wno-declaration-after-statement
      -Wno-zero-as-null-pointer-constant
      -Wno-implicit-int-float-conversion
      -Wno-double-promotion
      -Wno-unsafe-buffer-usage
      -Wno-deprecated-declarations
      -Wno-error=rtti
      -Wno-error=cast-function-type-strict
    )
  endif()
endif()

function(wasmedge_setup_target target)
  set_target_properties(${target} PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF
    CXX_VISIBILITY_PRESET hidden
    ENABLE_EXPORTS ON
    POSITION_INDEPENDENT_CODE ON
    VISIBILITY_INLINES_HIDDEN ON
    BUILD_RPATH_USE_ORIGIN ON
    MACOSX_RPATH ON
    INTERPROCEDURAL_OPTIMIZATION ${WASMEDGE_INTERPROCEDURAL_OPTIMIZATION}
  )
  if(WASMEDGE_BUILD_FUZZING)
    target_compile_definitions(${target}
      PUBLIC
      WASMEDGE_BUILD_FUZZING
    )
  endif()
  target_compile_options(${target}
    PRIVATE
    $<$<COMPILE_LANGUAGE:C>:${WASMEDGE_CFLAGS}>
    $<$<COMPILE_LANGUAGE:CXX>:${WASMEDGE_CFLAGS}>
  )

  if(WASMEDGE_ENABLE_UB_SANITIZER)
    target_link_options(${target}
      PRIVATE
      -fsanitize=undefined
    )
  endif()

  if(WASMEDGE_BUILD_FUZZING AND NOT DEFINED LIB_FUZZING_ENGINE)
    target_compile_options(${target}
      PUBLIC
      $<$<COMPILE_LANGUAGE:C>:-fsanitize=fuzzer,address>
      $<$<COMPILE_LANGUAGE:CXX>:-fsanitize=fuzzer,address>
    )
    target_link_options(${target}
      PUBLIC
      -fsanitize=fuzzer,address
    )
  endif()
endfunction()

function(wasmedge_add_library target)
  add_library(${target} ${ARGN})
  wasmedge_setup_target(${target})
  # Linux needs an explicit INSTALL_RPATH to allow libwasmedge.so to find
  # libwasiNNRPC.so in the same directory.
  if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set_target_properties(${target} PROPERTIES
      INSTALL_RPATH "$ORIGIN"
    )
  endif()
endfunction()

function(wasmedge_add_executable target)
  add_executable(${target} ${ARGN})
  wasmedge_setup_target(${target})
  file(RELATIVE_PATH rel /${CMAKE_INSTALL_BINDIR} /${CMAKE_INSTALL_LIBDIR})
  if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set_target_properties(${target} PROPERTIES
      INSTALL_RPATH "$ORIGIN/${rel}"
    )
  elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set_target_properties(${target} PROPERTIES
      INSTALL_RPATH "@executable_path/${rel}"
    )
  endif()
endfunction()

# Generate the list of static libs to statically link LLVM.
if((WASMEDGE_LINK_LLVM_STATIC OR WASMEDGE_BUILD_STATIC_LIB) AND WASMEDGE_USE_LLVM)
  # Pack the LLVM and lld static libraries.
  find_package(LLVM REQUIRED HINTS "${LLVM_DIR}")
  find_package(LLD HINTS "${LLVM_DIR}" "${LLD_DIR}")
  if(LLD_FOUND)
    get_property(LLD_LIBRARY_DIR TARGET lldELF PROPERTY IMPORTED_LOCATION_RELEASE)
    get_filename_component(LLD_LIBRARY_DIR "${LLD_LIBRARY_DIR}" DIRECTORY)
  endif()
  if(NOT IS_DIRECTORY "${LLD_LIBRARY_DIR}")
    set(LLD_LIBRARY_DIR ${LLVM_LIBRARY_DIR})
  endif()
  execute_process(
    COMMAND ${LLVM_BINARY_DIR}/bin/llvm-config --libs --link-static
    core lto native nativecodegen option passes support orcjit transformutils all-targets
    OUTPUT_VARIABLE WASMEDGE_LLVM_LINK_LIBS_NAME
  )
  string(REPLACE "-l" "" WASMEDGE_LLVM_LINK_LIBS_NAME "${WASMEDGE_LLVM_LINK_LIBS_NAME}")
  string(REGEX REPLACE "[\r\n]" "" WASMEDGE_LLVM_LINK_LIBS_NAME "${WASMEDGE_LLVM_LINK_LIBS_NAME}")
  string(REPLACE " " ";" WASMEDGE_LLVM_LINK_LIBS_NAME "${WASMEDGE_LLVM_LINK_LIBS_NAME}")
  set(WASMEDGE_LLVM_LINK_LIBS_NAME "${WASMEDGE_LLVM_LINK_LIBS_NAME}")

  list(APPEND WASMEDGE_LLVM_LINK_STATIC_COMPONENTS
    ${LLD_LIBRARY_DIR}/liblldELF.a
    ${LLD_LIBRARY_DIR}/liblldCommon.a
  )
  foreach(LIB_NAME IN LISTS WASMEDGE_LLVM_LINK_LIBS_NAME)
    list(APPEND WASMEDGE_LLVM_LINK_STATIC_COMPONENTS
      ${LLVM_LIBRARY_DIR}/lib${LIB_NAME}.a
    )
  endforeach()
  if(LLVM_VERSION_MAJOR LESS_EQUAL 13)
    # For LLVM <= 13
    list(APPEND WASMEDGE_LLVM_LINK_STATIC_COMPONENTS
      ${LLD_LIBRARY_DIR}/liblldCore.a
      ${LLD_LIBRARY_DIR}/liblldDriver.a
      ${LLD_LIBRARY_DIR}/liblldReaderWriter.a
      ${LLD_LIBRARY_DIR}/liblldYAML.a
    )
  else()
    # For LLVM 14
    list(APPEND WASMEDGE_LLVM_LINK_STATIC_COMPONENTS
      ${LLD_LIBRARY_DIR}/liblldMinGW.a
      ${LLD_LIBRARY_DIR}/liblldCOFF.a
      ${LLD_LIBRARY_DIR}/liblldMachO.a
      ${LLD_LIBRARY_DIR}/liblldWasm.a
    )
  endif()
  if(LLVM_VERSION_MAJOR GREATER_EQUAL 15)
    # For LLVM 15 or greater on MacOS, or all LLVM 16+
    if(APPLE OR LLVM_VERSION_MAJOR GREATER_EQUAL 16)
      find_package(zstd REQUIRED)
      get_filename_component(ZSTD_PATH "${zstd_LIBRARY}" DIRECTORY)
      list(APPEND WASMEDGE_LLVM_LINK_STATIC_COMPONENTS
        ${ZSTD_PATH}/libzstd.a
      )
    endif()
  endif()

  list(APPEND WASMEDGE_LLVM_LINK_SHARED_COMPONENTS
    dl
    pthread
  )
  if(APPLE)
    list(APPEND WASMEDGE_LLVM_LINK_SHARED_COMPONENTS
      ncurses
      z
      xar
    )
  elseif(UNIX)
    list(APPEND WASMEDGE_LLVM_LINK_SHARED_COMPONENTS
      rt
    )
    if(WASMEDGE_BUILD_STATIC_LIB)
      # Static library will forcefully turn off the LTO.
      # Therefore, libz and libtinfo can be statically linked.
      find_package(ZLIB REQUIRED)
      get_filename_component(ZLIB_PATH "${ZLIB_LIBRARIES}" DIRECTORY)
      list(APPEND WASMEDGE_LLVM_LINK_STATIC_COMPONENTS ${ZLIB_PATH}/libz.a)
      if(NOT WASMEDGE_DISABLE_LIBTINFO)
        list(APPEND WASMEDGE_LLVM_LINK_STATIC_COMPONENTS ${ZLIB_PATH}/libtinfo.a)
      endif()
    else()
      # If not build static lib, dynamic link libz and libtinfo.
      list(APPEND WASMEDGE_LLVM_LINK_SHARED_COMPONENTS
        z
      )
      if(NOT WASMEDGE_DISABLE_LIBTINFO)
        list(APPEND WASMEDGE_LLVM_LINK_SHARED_COMPONENTS tinfo)
      endif()
    endif()
  endif()
endif()

function(wasmedge_setup_simdjson)
  if(TARGET simdjson::simdjson)
    return()
  endif()
  # setup simdjson
  if(NOT WASMEDGE_FORCE_DOWNLOAD_SIMDJSON)
    message(STATUS "Checking SIMDJSON from system")
    find_package(simdjson QUIET)
  endif()
  if(simdjson_FOUND)
    message(STATUS "SIMDJSON found")
  else()
    include(FetchContent)
    message(STATUS "Downloading SIMDJSON source")
    FetchContent_Declare(
      simdjson
      GIT_REPOSITORY https://github.com/simdjson/simdjson.git
      GIT_TAG  tags/v3.10.0
      GIT_SHALLOW TRUE
    )
    set(SIMDJSON_DEVELOPER_MODE OFF CACHE BOOL "SIMDJSON developer mode" FORCE)
    FetchContent_MakeAvailable(simdjson)
    set_property(TARGET simdjson PROPERTY POSITION_INDEPENDENT_CODE ON)
    message(STATUS "Downloading SIMDJSON source -- done")

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      target_compile_options(simdjson
        PUBLIC
        -Wno-undef
        -Wno-suggest-override
        -Wno-documentation
        -Wno-sign-conversion
        -Wno-extra-semi-stmt
        -Wno-old-style-cast
        -Wno-error=unused-parameter
        -Wno-error=unused-template
        -Wno-conditional-uninitialized
        -Wno-implicit-int-conversion
        -Wno-shorten-64-to-32
        -Wno-range-loop-bind-reference
        -Wno-format-nonliteral
        -Wno-unused-exception-parameter
        -Wno-unused-macros
        -Wno-unused-member-function
        -Wno-missing-prototypes
      )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
      target_compile_options(simdjson
        PUBLIC
        $<$<COMPILE_LANGUAGE:C,CXX>:/wd4100> # unreferenced formal parameter
        $<$<COMPILE_LANGUAGE:C,CXX>:/wd4505> # unreferenced local function has been removed
      )
    endif()
  endif()
endfunction()

function(wasmedge_setup_spdlog)
  if(TARGET spdlog::spdlog)
    return()
  endif()
  # setup spdlog
  find_package(spdlog QUIET)
  if(spdlog_FOUND)
    message(STATUS "spdlog found")
  else()
    include(FetchContent)
    message(STATUS "Downloading fmt source")
    FetchContent_Declare(
      fmt
      GIT_REPOSITORY https://github.com/fmtlib/fmt.git
      GIT_TAG        11.0.2
      GIT_SHALLOW    TRUE
    )
    set(FMT_INSTALL OFF CACHE BOOL "Generate the install target." FORCE)
    FetchContent_MakeAvailable(fmt)
    message(STATUS "Downloading fmt source -- done")
    wasmedge_setup_target(fmt)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      target_compile_options(fmt
        PUBLIC
        -Wno-missing-noreturn
        PRIVATE
        -Wno-sign-conversion
      )
    endif()
    if (WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      target_compile_options(fmt
        PUBLIC
        -Wno-duplicate-enum
      )
    endif()

    message(STATUS "Downloading spdlog source")
    FetchContent_Declare(
      spdlog
      GIT_REPOSITORY https://github.com/gabime/spdlog.git
      GIT_TAG        v1.13.0
      GIT_SHALLOW    TRUE
    )
    set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "Build shared library" FORCE)
    set(SPDLOG_FMT_EXTERNAL ON  CACHE BOOL "Use external fmt library instead of bundled" FORCE)
    FetchContent_MakeAvailable(spdlog)
    message(STATUS "Downloading spdlog source -- done")
    wasmedge_setup_target(spdlog)
  endif()
endfunction()

function(wasmedge_setup_stb_image)
  if(TARGET wasmedgeDepsSTBImage)
    return()
  endif()
  # setup stb_image
  include(FetchContent)
  message(STATUS "Downloading stb_image source")
  FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        2dfbe86bef853be33cbbda07abcb4db58c7f817d
    GIT_SHALLOW    FALSE
  )
  FetchContent_MakeAvailable(stb)
  message(STATUS "Downloading stb_image source -- done")
  add_library(wasmedgeDepsSTBImage INTERFACE)
  target_include_directories(wasmedgeDepsSTBImage SYSTEM INTERFACE ${stb_SOURCE_DIR})
endfunction()
