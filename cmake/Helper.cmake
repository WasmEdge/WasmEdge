# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

set(WASMEDGE_INTERPROCEDURAL_OPTIMIZATION OFF)
if(CMAKE_BUILD_TYPE STREQUAL Release OR CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo)
  if(NOT WASMEDGE_FORCE_DISABLE_LTO)
    set(WASMEDGE_INTERPROCEDURAL_OPTIMIZATION ON)
  endif()
  if(CMAKE_GENERATOR STREQUAL Ninja)
    if(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
      list(TRANSFORM CMAKE_C_COMPILE_OPTIONS_IPO REPLACE "^-flto$" "-flto=auto")
      list(TRANSFORM CMAKE_CXX_COMPILE_OPTIONS_IPO REPLACE "^-flto$" "-flto=auto")
    endif()
    set(CMAKE_JOB_POOLS "link=2")
    set(CMAKE_JOB_POOL_LINK link)
  endif()
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
    /bigobj # for large object files
  )
  # /W4 already enables the C4456-C4459 shadow warnings, so no extra enables
  # are needed; pair them with suppression flags to keep third-party targets
  # exempt on MSVC as well.
  set(WASMEDGE_SHADOW_SUPPRESS_FLAGS /wd4456 /wd4457 /wd4458 /wd4459)
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
    -Wno-error=unused-command-line-argument
    -Wno-error=unknown-warning-option
    -Wno-ctad-maybe-unsupported
    -Wno-gnu-anonymous-struct
    -Wno-keyword-macro
    -Wno-language-extension-token
    -Wno-missing-noreturn
    -Wno-newline-eof
    -Wno-signed-enum-bitfield
    -Wno-switch-enum
    -Wno-undefined-func-template
  )

  # -Wshadow is not documented to enable -Wshadow-field-in-constructor, so
  # the Name(Name) constructor idiom should stay accepted, but clang-cl 21 on
  # Windows fires it under these flags anyway; disable it explicitly and
  # re-enable the narrower -Wshadow-field-in-constructor-modified check that
  # -Wshadow intends. -Wshadow-field is not part of -Wshadow either; it is
  # enabled explicitly to catch declarations that shadow members inherited
  # from a base class.
  list(APPEND WASMEDGE_CFLAGS
    -Wshadow
    -Wshadow-field
    -Wno-shadow-field-in-constructor
    -Wshadow-field-in-constructor-modified
  )
  set(WASMEDGE_SHADOW_SUPPRESS_FLAGS -Wno-shadow -Wno-shadow-field)

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

  if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 22.0.0)
    list(APPEND WASMEDGE_CFLAGS
      -Wno-shadow-header
    )
  endif()

  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 13.0.0)
    list(APPEND WASMEDGE_CFLAGS
      -Wno-error=return-std-move-in-c++11
    )
  elseif(NOT CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    list(APPEND WASMEDGE_CFLAGS
      -Wno-reserved-identifier
    )
  endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  # GCC's -Wshadow, unlike Clang's, flags the Name(Name) constructor idiom
  # and offers no subgroup to exempt it; =local checks local shadowing only.
  list(APPEND WASMEDGE_CFLAGS
    -Wshadow=local
  )
  # Match the enabled subgroup so third-party targets drop -Wshadow=local.
  set(WASMEDGE_SHADOW_SUPPRESS_FLAGS -Wno-shadow=local)
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
      -Wno-padded
      -Wno-error=nrvo
      -Wno-unique-object-duplication
      -Wno-deprecated-declarations
      -Wno-error=rtti
      -Wno-error=cast-function-type-strict
      -Wno-error=c++-keyword
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

# Third-party targets built with WASMEDGE_CFLAGS are not ours to fix; undo the
# shadow warning enables on them.
function(wasmedge_suppress_shadow_warnings)
  if(NOT WASMEDGE_SHADOW_SUPPRESS_FLAGS)
    return()
  endif()
  set(SHADOW_SUPPRESS_OPTIONS)
  foreach(flag IN LISTS WASMEDGE_SHADOW_SUPPRESS_FLAGS)
    list(APPEND SHADOW_SUPPRESS_OPTIONS "$<$<COMPILE_LANGUAGE:C,CXX>:${flag}>")
  endforeach()
  foreach(target IN LISTS ARGN)
    target_compile_options(${target} PRIVATE ${SHADOW_SUPPRESS_OPTIONS})
  endforeach()
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

# Download a file with hash verification, retrying on transient failures.
# HASH uses the same ALGO=value format as file(DOWNLOAD EXPECTED_HASH).
# Set the HF_TOKEN environment variable to authenticate Hugging Face
# downloads and avoid anonymous rate limits.
function(wasmedge_download URL OUTPUT HASH)
  string(REPLACE "=" ";" HASH_PARTS "${HASH}")
  list(GET HASH_PARTS 0 HASH_ALGORITHM)
  list(GET HASH_PARTS 1 HASH_EXPECTED)
  string(TOLOWER "${HASH_EXPECTED}" HASH_EXPECTED)
  if(EXISTS "${OUTPUT}")
    file(${HASH_ALGORITHM} "${OUTPUT}" HASH_ACTUAL)
    if(HASH_ACTUAL STREQUAL HASH_EXPECTED)
      message(STATUS "Skipping download of ${URL}: ${OUTPUT} is up-to-date")
      return()
    endif()
    file(REMOVE "${OUTPUT}")
  endif()
  set(DOWNLOAD_EXTRA_ARGS "")
  if(URL MATCHES "^https://huggingface\\.co/" AND NOT "$ENV{HF_TOKEN}" STREQUAL "")
    list(APPEND DOWNLOAD_EXTRA_ARGS HTTPHEADER "Authorization: Bearer $ENV{HF_TOKEN}")
  endif()
  set(RETRY_DELAYS 0 15 60)
  foreach(RETRY_DELAY IN LISTS RETRY_DELAYS)
    if(RETRY_DELAY GREATER 0)
      message(STATUS "Retrying download of ${URL} in ${RETRY_DELAY} seconds")
      execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep ${RETRY_DELAY})
    endif()
    file(DOWNLOAD "${URL}" "${OUTPUT}"
      SHOW_PROGRESS
      INACTIVITY_TIMEOUT 60
      STATUS DOWNLOAD_STATUS
      LOG DOWNLOAD_LOG
      ${DOWNLOAD_EXTRA_ARGS}
    )
    list(GET DOWNLOAD_STATUS 0 DOWNLOAD_CODE)
    list(GET DOWNLOAD_STATUS 1 DOWNLOAD_MESSAGE)
    if(DOWNLOAD_CODE EQUAL 0)
      file(${HASH_ALGORITHM} "${OUTPUT}" HASH_ACTUAL)
      if(HASH_ACTUAL STREQUAL HASH_EXPECTED)
        return()
      endif()
      set(DOWNLOAD_MESSAGE "${HASH_ALGORITHM} mismatch: expected ${HASH_EXPECTED}, got ${HASH_ACTUAL}")
    endif()
    if(NOT "$ENV{HF_TOKEN}" STREQUAL "")
      string(REPLACE "$ENV{HF_TOKEN}" "<redacted>" DOWNLOAD_LOG "${DOWNLOAD_LOG}")
    endif()
    string(REGEX MATCHALL "HTTP/[0-9.]+ [0-9]+" HTTP_STATUS_LINES "${DOWNLOAD_LOG}")
    if(HTTP_STATUS_LINES)
      list(GET HTTP_STATUS_LINES -1 LAST_HTTP_STATUS)
      string(APPEND DOWNLOAD_MESSAGE " (${LAST_HTTP_STATUS})")
    else()
      string(LENGTH "${DOWNLOAD_LOG}" DOWNLOAD_LOG_LENGTH)
      if(DOWNLOAD_LOG_LENGTH GREATER 512)
        math(EXPR DOWNLOAD_LOG_OFFSET "${DOWNLOAD_LOG_LENGTH} - 512")
        string(SUBSTRING "${DOWNLOAD_LOG}" ${DOWNLOAD_LOG_OFFSET} -1 DOWNLOAD_LOG)
      endif()
      string(APPEND DOWNLOAD_MESSAGE "\nTransfer log tail:\n${DOWNLOAD_LOG}")
    endif()
    message(WARNING "Downloading ${URL} failed: ${DOWNLOAD_MESSAGE}")
    file(REMOVE "${OUTPUT}")
  endforeach()
  message(FATAL_ERROR "Failed to download ${URL} after 3 attempts")
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
    core linker lto native nativecodegen option passes support orcjit transformutils all-targets
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
    # For LLVM 15 or greater on macOS, or all LLVM 16+
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

# Re-export a target's interface include directories as system includes, so that
# consumers reach its headers through -isystem / -external:I instead of -I and
# never compile them with the project warning flags. The SYSTEM target property
# would do this directly, but it needs CMake 3.25 and the floor here is 3.18.
function(wasmedge_mark_system_includes target)
  if(NOT TARGET ${target})
    return()
  endif()
  get_target_property(WASMEDGE_ALIASED ${target} ALIASED_TARGET)
  if(WASMEDGE_ALIASED)
    set(target ${WASMEDGE_ALIASED})
  endif()
  get_target_property(WASMEDGE_INCLUDE_DIRS
    ${target} INTERFACE_INCLUDE_DIRECTORIES)
  if(NOT WASMEDGE_INCLUDE_DIRS)
    return()
  endif()
  get_target_property(WASMEDGE_SYSTEM_INCLUDE_DIRS
    ${target} INTERFACE_SYSTEM_INCLUDE_DIRECTORIES)
  if(NOT WASMEDGE_SYSTEM_INCLUDE_DIRS)
    set(WASMEDGE_SYSTEM_INCLUDE_DIRS)
  endif()
  list(APPEND WASMEDGE_SYSTEM_INCLUDE_DIRS ${WASMEDGE_INCLUDE_DIRS})
  list(REMOVE_DUPLICATES WASMEDGE_SYSTEM_INCLUDE_DIRS)
  set_property(TARGET ${target} PROPERTY
    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ${WASMEDGE_SYSTEM_INCLUDE_DIRS})
endfunction()

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
      GIT_TAG  1bcf71bd85059ab6574ea1159de9298dcc1212c5  # v4.6.4
      GIT_SHALLOW TRUE
    )
    set(SIMDJSON_DEVELOPER_MODE OFF CACHE BOOL "SIMDJSON developer mode" FORCE)
    FetchContent_MakeAvailable(simdjson)
    set_property(TARGET simdjson PROPERTY POSITION_INDEPENDENT_CODE ON)
    message(STATUS "Downloading SIMDJSON source -- done")

    # Consumers pull in simdjson through -isystem / -external:I so that its
    # headers are never compiled with the project warning flags, the same way
    # gtest is already treated. The suppressions below then only have to cover
    # simdjson's own sources, which inherit WASMEDGE_CFLAGS from the enclosing
    # directory scope.
    wasmedge_mark_system_includes(simdjson)

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      target_compile_options(simdjson
        PRIVATE
        -Wno-undef
        -Wno-suggest-override
        -Wno-documentation
        -Wno-sign-conversion
        -Wno-extra-semi-stmt
        -Wno-old-style-cast
        -Wno-unused-parameter
        -Wno-unused-template
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
        PRIVATE
        $<$<COMPILE_LANGUAGE:C,CXX>:/wd4100> # unreferenced formal parameter
        $<$<COMPILE_LANGUAGE:C,CXX>:/wd4505> # unreferenced local function has been removed
      )
    endif()
    wasmedge_suppress_shadow_warnings(simdjson)
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
    wasmedge_mark_system_includes(spdlog::spdlog)
    wasmedge_mark_system_includes(fmt::fmt)
    wasmedge_mark_system_includes(fmt::fmt-header-only)
  else()
    include(FetchContent)
    message(STATUS "Downloading fmt source")
    # Held at 12.1.0. fmt 12.2.0 needs CMake 3.19 or later, which this project
    # does not require: it sets VERSION, SOVERSION and DEBUG_POSTFIX on the
    # fmt-header-only INTERFACE target, and CMake rejects non-whitelisted
    # properties on INTERFACE libraries before 3.19. Debian bullseye, which
    # builds the static release, carries CMake 3.18. fmt 12.2.0 also fails to
    # compile its own 128-bit fallback in do_format_decimal, which every target
    # without a native __int128 -- MSVC among them -- instantiates.
    FetchContent_Declare(
      fmt
      GIT_REPOSITORY https://github.com/fmtlib/fmt.git
      GIT_TAG        407c905e45ad75fc29bf0f9bb7c5c2fd3475976f  # 12.1.0
      GIT_SHALLOW    TRUE
    )
    set(FMT_INSTALL OFF CACHE BOOL "Generate the install target." FORCE)
    set(FMT_SYSTEM_HEADERS ON CACHE BOOL "Expose headers with marking them as system." FORCE)
    FetchContent_MakeAvailable(fmt)
    message(STATUS "Downloading fmt source -- done")
    wasmedge_setup_target(fmt)
    wasmedge_mark_system_includes(fmt)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      target_compile_options(fmt
        PRIVATE
        -Wno-missing-noreturn
        -Wno-sign-conversion
      )
    endif()
    wasmedge_suppress_shadow_warnings(fmt)
    if (WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      target_compile_options(fmt
        PRIVATE
        -Wno-duplicate-enum
        -Wno-padded
        -Wno-error=unique-object-duplication
        -Wno-error=nrvo
      )
    endif()

    message(STATUS "Downloading spdlog source")
    FetchContent_Declare(
      spdlog
      GIT_REPOSITORY https://github.com/gabime/spdlog.git
      GIT_TAG        79524ddd08a4ec981b7fea76afd08ee05f83755d  # v1.17.0
      GIT_SHALLOW    TRUE
    )
    set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "Build shared library" FORCE)
    set(SPDLOG_FMT_EXTERNAL ON  CACHE BOOL "Use external fmt library instead of bundled" FORCE)
    set(SPDLOG_SYSTEM_INCLUDES ON CACHE BOOL "Include as system headers (skip for clang-tidy)." FORCE)
    FetchContent_MakeAvailable(spdlog)
    message(STATUS "Downloading spdlog source -- done")
    wasmedge_setup_target(spdlog)
    wasmedge_mark_system_includes(spdlog)
    wasmedge_suppress_shadow_warnings(spdlog)
    if (WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      target_compile_options(spdlog
        PRIVATE
        -Wno-unique-object-duplication
      )
    endif()
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
    GIT_TAG        31c1ad37456438565541f4919958214b6e762fb4  # stb_image v2.30, stb_image_resize2 v2.18
    GIT_SHALLOW    FALSE
  )
  FetchContent_MakeAvailable(stb)
  message(STATUS "Downloading stb_image source -- done")
  add_library(wasmedgeDepsSTBImage INTERFACE)
  target_include_directories(wasmedgeDepsSTBImage SYSTEM INTERFACE ${stb_SOURCE_DIR})
  wasmedge_mark_system_includes(wasmedgeDepsSTBImage)
endfunction()
