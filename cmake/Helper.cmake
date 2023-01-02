# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

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

list(APPEND WASMEDGE_CFLAGS
  -Wall
  -Wextra
  -Werror
  -Wno-error=pedantic
)
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  list(APPEND WASMEDGE_CFLAGS -Wno-psabi)
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
    -Wno-newline-eof
    -Wno-shadow-field-in-constructor
    -Wno-signed-enum-bitfield
    -Wno-switch-enum
    -Wno-undefined-func-template
  )
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
endif()

if(WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_ENABLE_EXTENDED_ALIGNED_STORAGE -DNOMINMAX -D_ITERATOR_DEBUG_LEVEL=0)
  list(APPEND WASMEDGE_CFLAGS
    "/EHa"
    -Wno-c++98-compat
    -Wno-c++98-compat-pedantic
    -Wno-exit-time-destructors
    -Wno-global-constructors
    -Wno-used-but-marked-unused
    -Wno-nonportable-system-include-path
    -Wno-float-equal
    -Wno-declaration-after-statement
    -Wno-zero-as-null-pointer-constant
  )
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
    ${WASMEDGE_CFLAGS}
  )
  if(WASMEDGE_BUILD_FUZZING AND NOT DEFINED LIB_FUZZING_ENGINE)
    target_compile_options(${target}
      PUBLIC
      -fsanitize=fuzzer,address
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
if((WASMEDGE_LINK_LLVM_STATIC OR WASMEDGE_BUILD_STATIC_LIB) AND WASMEDGE_BUILD_AOT_RUNTIME)
  # Pack the LLVM and lld static libraries.
  find_package(LLVM REQUIRED HINTS "${LLVM_CMAKE_PATH}")
  execute_process(
    COMMAND ${LLVM_BINARY_DIR}/bin/llvm-config --libs --link-static
    core lto native nativecodegen option passes support transformutils all-targets
    OUTPUT_VARIABLE WASMEDGE_LLVM_LINK_LIBS_NAME
  )
  string(REPLACE "-l" "" WASMEDGE_LLVM_LINK_LIBS_NAME ${WASMEDGE_LLVM_LINK_LIBS_NAME})
  string(REGEX REPLACE "[\r\n]" "" WASMEDGE_LLVM_LINK_LIBS_NAME ${WASMEDGE_LLVM_LINK_LIBS_NAME})
  string(REPLACE " " "\;" WASMEDGE_LLVM_LINK_LIBS_NAME ${WASMEDGE_LLVM_LINK_LIBS_NAME})
  set(WASMEDGE_LLVM_LINK_LIBS_NAME ${WASMEDGE_LLVM_LINK_LIBS_NAME})


  list(APPEND WASMEDGE_LLVM_LINK_STATIC_COMPONENTS
    ${LLVM_LIBRARY_DIR}/liblldELF.a
    ${LLVM_LIBRARY_DIR}/liblldCommon.a
  )
  foreach(LIB_NAME IN LISTS WASMEDGE_LLVM_LINK_LIBS_NAME)
    list(APPEND WASMEDGE_LLVM_LINK_STATIC_COMPONENTS
      ${LLVM_LIBRARY_DIR}/lib${LIB_NAME}.a
    )
  endforeach()
  if(LLVM_VERSION_MAJOR LESS_EQUAL 13)
    # For LLVM <= 13
    list(APPEND WASMEDGE_LLVM_LINK_STATIC_COMPONENTS
      ${LLVM_LIBRARY_DIR}/liblldCore.a
      ${LLVM_LIBRARY_DIR}/liblldDriver.a
      ${LLVM_LIBRARY_DIR}/liblldReaderWriter.a
      ${LLVM_LIBRARY_DIR}/liblldYAML.a
    )
  else()
    # For LLVM 14
    list(APPEND WASMEDGE_LLVM_LINK_STATIC_COMPONENTS
      ${LLVM_LIBRARY_DIR}/liblldMinGW.a
      ${LLVM_LIBRARY_DIR}/liblldCOFF.a
      ${LLVM_LIBRARY_DIR}/liblldMachO.a
      ${LLVM_LIBRARY_DIR}/liblldWasm.a
    )
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
      list(APPEND WASMEDGE_LLVM_LINK_STATIC_COMPONENTS
        ${ZLIB_PATH}/libz.a
        ${ZLIB_PATH}/libtinfo.a
      )
    else()
      # If not build static lib, dynamic link libz and libtinfo.
      list(APPEND WASMEDGE_LLVM_LINK_SHARED_COMPONENTS
        z
        tinfo
      )
    endif()
  endif()
endif()
