# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

# Try to get libbpf use the following order
# - PkgConfig
# - ${LIBBPF_SOURCE_DIR}
# - FetchContent

option(WASMEDGE_PLUGIN_WASM_BPF_BUILD_LIBBPF_WITH_PKG_CONF "Configure libbpf to use pkg-config for the build process. If enabled, the libbpf build script will utilize pkg-config to search for dependencies such as libz and libelf. If this feature is disabled, the headers and binaries for libz and libelf need to be correctly positioned." YES)

message(STATUS "Trying to get libbpf..")
message(STATUS "Build libbpf with pkg-config: ${WASMEDGE_PLUGIN_WASM_BPF_BUILD_LIBBPF_WITH_PKG_CONF}")
set(LIBBPF_FOUND FALSE)

# A wrapper function to add libbpf located at a local path as a dependency
function(AddLibbpfAsExternal SOURCE_ROOT WITH_PKG_CONF)
  include(ExternalProject)
  set(LIBBPF_SO_PATH ${SOURCE_ROOT}/src/build/libbpf.so)
  set(LIBBPF_INCLUDE_DIRS_LOCAL "${SOURCE_ROOT}/src/root/usr/include" "${SOURCE_ROOT}/include/uapi" "${SOURCE_ROOT}/include")
  set(LIBBPF_INCLUDE_DIRS ${LIBBPF_INCLUDE_DIRS_LOCAL} PARENT_SCOPE)

  set(LIBBPF_LIBRARIES ${LIBBPF_SO_PATH} PARENT_SCOPE)
  set(LIBBPF_LIBRARIES_STATIC ${SOURCE_ROOT}/src/build/libbpf.a PARENT_SCOPE)

  if(${WITH_PKG_CONF})
    set(PKGCONF_PREFIX "")
  else()
    set(PKGCONF_PREFIX "NO_PKG_CONFIG=1")
    set(LIBBPF_DEP_LIBRARIES "elf" "z" PARENT_SCOPE)
  endif()
  message(STATUS "SOURCE_ROOT=${SOURCE_ROOT}")
  ExternalProject_Add(libbpf
    PREFIX libbpf
    SOURCE_DIR ${SOURCE_ROOT}
    CONFIGURE_COMMAND "mkdir" "build" "root"
    BUILD_COMMAND "${PKGCONF_PREFIX}" "OBJDIR=${SOURCE_ROOT}/src/build" "DESTDIR=${SOURCE_ROOT}/src/root" "CFLAGS=-fPIC" "make" "-C" "${SOURCE_ROOT}/src" "install"
    INSTALL_COMMAND "cp" "${LIBBPF_SO_PATH}" "${CMAKE_CURRENT_BINARY_DIR}/libbpf.so"
    BUILD_IN_SOURCE TRUE
    BUILD_BYPRODUCTS ${LIBBPF_SO_PATH} ${SOURCE_ROOT}/src/build/libbpf.a
  )

  set(LIBBPF_TARGET_NAME libbpf PARENT_SCOPE)
endfunction()

# Try PkgConfig
if(NOT ${LIBBPF_FOUND})
  find_package(PkgConfig)

  if(PkgConfig_FOUND)
    message(STATUS "Try to get libbpf through PkgConfig")

    # It will set LIBBPF_FOUND for us
    pkg_check_modules(LIBBPF libbpf>=1.2.0 IMPORTED_TARGET)
    set(LIBBPF_TARGET_NAME "PkgConfig::LIBBPF")
    message(STATUS "LIBBPF_FOUND=${LIBBPF_FOUND}")

    if(${LIBBPF_FOUND})
      SET(LIBBPF_FOUND TRUE)
    else()
      SET(LIBBPF_FOUND FALSE)
    endif()

    if(${LIBBPF_FOUND})
      message(STATUS "libbpf found using PkgConfig")
      set(LIBBPF_SOURCE "pkgconf")
    else()
      message(STATUS "libbpf not found using pkgconfig")
    endif()
  else()
    message(STATUS "PkgConfig not found")
  endif()
endif()

# Try LIBBPF_SOURCE_DIR
if(NOT ${LIBBPF_FOUND})
  message(STATUS "Try to get libbpf through the pre-defined LIBBPF_SOURCE_DIR")

  if(DEFINED LIBBPF_SOURCE_DIR)
    AddLibbpfAsExternal(${LIBBPF_SOURCE_DIR} ${WASMEDGE_PLUGIN_WASM_BPF_BUILD_LIBBPF_WITH_PKG_CONF})
    set(LIBBPF_FOUND TRUE)
    message(STATUS "libbpf found using LIBBPF_SOURCE_DIR")
    set(LIBBPF_SOURCE "sourcedir")
  else()
    message(STATUS "LIBBPF_SOURCE_DIR not defined")
  endif()
endif()

# Try FetchContent
if(NOT ${LIBBPF_FOUND})
  message(STATUS "Try to get libbpf through FetchContent")
  include(FetchContent)
  FetchContent_Declare(
    libbpf
    GIT_REPOSITORY https://github.com/libbpf/libbpf
    GIT_TAG cf46d44f0a06aa8b9400691ea3eb86ca4f066d5c
  )
  FetchContent_GetProperties(libbpf)

  if(NOT libbpf_POPULATED)
    message(STATUS "Fetching libbpf..")
    FetchContent_Populate(libbpf)
    message(STATUS "Fetched libbpf")
  endif()

  set(LIBBPF_DOWNLOAD_SOURCE_DIR "${libbpf_SOURCE_DIR}")
  message(DEBUG "libbpf saved at: ${LIBBPF_DOWNLOAD_SOURCE_DIR}")
  AddLibbpfAsExternal(${LIBBPF_DOWNLOAD_SOURCE_DIR} ${WASMEDGE_PLUGIN_WASM_BPF_BUILD_LIBBPF_WITH_PKG_CONF})
  set(LIBBPF_FOUND TRUE)
  set(LIBBPF_SOURCE "fetch-content")
endif()

# If we cannot find libbpf..
if(NOT ${LIBBPF_FOUND})
  message(FATAL_ERROR "Could not find libbpf")
endif()

if(${WASMEDGE_PLUGIN_WASM_BPF_BUILD_LIBBPF_WITH_PKG_CONF})
  # Find the dependencies `libelf` and `libz` of libbpf
  find_package(PkgConfig)

  pkg_check_modules(LIBBPF_DEP REQUIRED libelf zlib)

  message(STATUS "(From PKGCONF) LIBBPF_DEP_LIBRARIES=${LIBBPF_DEP_LIBRARIES}")
endif()

message(STATUS "LIBBPF_INCLUDE_DIRS=${LIBBPF_INCLUDE_DIRS}")
message(STATUS "LIBBPF_LIBRARIES=${LIBBPF_LIBRARIES}")
message(STATUS "LIBBPF_TARGET_NAME=${LIBBPF_TARGET_NAME}")
message(STATUS "LIBBPF_LIBRARIES_STATIC=${LIBBPF_LIBRARIES_STATIC}")
message(STATUS "LIBBPF_SOURCE=${LIBBPF_SOURCE}")
message(STATUS "LIBBPF_DEP_LIBRARIES=${LIBBPF_DEP_LIBRARIES}")
message(STATUS "LIBBPF_DEP_LIBRARIES_STATIC=${LIBBPF_DEP_LIBRARIES_STATIC}")

wasmedge_add_library(wasmedgePluginWasmBpf
  SHARED
  wasm-bpf-module.cpp
  func-load-bpf-object.cpp
  func-close-bpf-object.cpp
  func-attach-bpf-program.cpp
  func-bpf-buffer-poll.cpp
  func-bpf-map-fd-by-name.cpp
  func-bpf-map-operate.cpp
  wasm-bpf.cpp
  util.cpp
)

add_dependencies(wasmedgePluginWasmBpf ${LIBBPF_TARGET_NAME})

if("${LIBBPF_SOURCE}" STREQUAL "pkgconf")
  message(STATUS "Link libbpf dynamically")
  target_link_libraries(wasmedgePluginWasmBpf PUBLIC ${LIBBPF_LIBRARIES} ${LIBBPF_DEP_LIBRARIES})
else()
  # Link libbpf statically if we don't use pkgconf, because under this case libbpf is not installed systemwide
  message(STATUS "Link libbpf statically")
  target_link_libraries(wasmedgePluginWasmBpf PUBLIC ${LIBBPF_LIBRARIES_STATIC} ${LIBBPF_DEP_LIBRARIES})
endif()

target_include_directories(wasmedgePluginWasmBpf PUBLIC ${LIBBPF_INCLUDE_DIRS})


set_target_properties(wasmedgePluginWasmBpf PROPERTIES
  CXX_STANDARD 17
  # Allow tests accessing plugin class functions
  CXX_VISIBILITY_PRESET default
  VISIBILITY_INLINES_HIDDEN OFF
)
# Fix undefined reference issue of `fmt::v9::formatter<WasmEdge::ErrInfo::InfoBoundary, char, void>::format(WasmEdge::ErrInfo::InfoBoundary const&, fmt::v9::basic_format_context<fmt::v9::appender, char>&) const`
target_link_libraries(wasmedgePluginWasmBpf
  PUBLIC wasmedgeCommon
)

target_compile_options(wasmedgePluginWasmBpf
  PUBLIC
  -DWASMEDGE_PLUGIN
  -fPIC
)

target_include_directories(wasmedgePluginWasmBpf
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${LIBBPF_INCLUDE_DIRS}
)

if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgePluginWasmBpf
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgePluginWasmBpf
    PRIVATE
    wasmedge_shared
  )
endif()
