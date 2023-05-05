# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

# Try to get libbpf use the following order
# - PkgConfig
# - ${LIBBPF_SOURCE_DIR}
# - FetchContent

message(STATUS "Trying to get libbpf..")
set(LIBBPF_FOUND FALSE)

# A wrapper function to add libbpf located at a local path as a dependency
function(AddLibbpfAsExternal SOURCE_ROOT)
  include(ExternalProject)
  ExternalProject_Add(libbpf
    PREFIX libbpf
    SOURCE_DIR ${SOURCE_ROOT}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND "make" "-C" "${SOURCE_ROOT}/src"
    INSTALL_COMMAND ""
    BUILD_IN_SOURCE TRUE
  )
  set(LIBBPF_SO_PATH ${SOURCE_ROOT}/src/libbpf.so)
  set(LIBBPF_INCLUDE_DIRS ${SOURCE_ROOT}/src PARENT_SCOPE)
  set(LIBBPF_LIBRARIES ${LIBBPF_SO_PATH} PARENT_SCOPE)
  set(LIBBPF_TARGET_NAME libbpf PARENT_SCOPE)
  file(COPY_FILE ${LIBBPF_SO_PATH} ${CMAKE_CURRENT_BINARY_DIR}/libbpf.so)

  # Copy libbpf.so to the place where libwasmedgePluginWasmBpf.so exists
  message(STATUS "Copied libbpf.so from ${LIBBPF_SO_PATH} to ${CMAKE_CURRENT_BINARY_DIR}/libbpf.so")
endfunction()

# Try PkgConfig
if(NOT ${LIBBPF_FOUND})
  find_package(PkgConfig)

  if(PkgConfig_FOUND)
    message(STATUS "Try to get libbpf through PkgConfig")

    # It will set LIBBPF_FOUND for us
    pkg_check_modules(LIBBPF libbpf>=1.2 IMPORTED_TARGET)
    set(LIBBPF_TARGET_NAME "PkgConfig::LIBBPF")

    if(${LIBBPF_FOUND})
      message(STATUS "libbpf found using PkgConfig")
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
    AddLibbpfAsExternal(${LIBBPF_SOURCE_DIR})
    set(LIBBPF_FOUND TRUE)
    message(STATUS "libbpf found using LIBBPF_SOURCE_DIR")
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
  AddLibbpfAsExternal(${LIBBPF_DOWNLOAD_SOURCE_DIR})
  set(LIBBPF_FOUND TRUE)
endif()

# If we cannot find libbpf..
if(NOT ${LIBBPF_FOUND})
  message(FATAL_ERROR "Could not find libbpf")
endif()

message(DEBUG "LIBBPF_INCLUDE_DIRS=${LIBBPF_INCLUDE_DIRS}")
message(DEBUG "LIBBPF_LIBRARIES=${LIBBPF_LIBRARIES}")
message(DEBUG "LIBBPF_TARGET_NAME=${LIBBPF_TARGET_NAME}")

# Find the dependencies `libelf` and `libz` of libbpf
find_package(PkgConfig)

pkg_check_modules(LIBBPF_DEP REQUIRED libelf zlib)

message(DEBUG "LIBBPF_DEP_LIBRARIES=${LIBBPF_DEP_LIBRARIES}")

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
target_link_libraries(wasmedgePluginWasmBpf PUBLIC ${LIBBPF_LIBRARIES} ${LIBBPF_DEP_LIBRARIES})
target_include_directories(wasmedgePluginWasmBpf PUBLIC ${LIBBPF_INCLUDE_DIRS})

set_target_properties(wasmedgePluginWasmBpf PROPERTIES
  CXX_STANDARD 17
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
    ${LIBBPF_LIBRARIES}
  )
else()
  target_link_libraries(wasmedgePluginWasmBpf
    PRIVATE
    wasmedge_shared
    ${LIBBPF_LIBRARIES}
  )
endif()
