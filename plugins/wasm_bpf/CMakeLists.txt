# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

if (DEFINED LIBBPF_SOURCE_DIR)
  verbose_message("Use libbpf source from ${LIBBPF_SOURCE_DIR}")
else()
  set(LIBBPF_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libbpf/src)
endif()

# Build vendored libbpf from github
include(ExternalProject)
ExternalProject_Add(
  libbpf
  PREFIX libbpf
  GIT_REPOSITORY https://github.com/libbpf/libbpf
  GIT_TAG        cf46d44f0a06aa8b9400691ea3eb86ca4f066d5c
  CONFIGURE_COMMAND ""
  BUILD_COMMAND cd src && make
    BUILD_STATIC_ONLY=1
    OBJDIR=${CMAKE_CURRENT_BINARY_DIR}/libbpf/libbpf
    DESTDIR=${CMAKE_CURRENT_BINARY_DIR}/libbpf
    INCLUDEDIR=
    LIBDIR=
    UAPIDIR=
    install install_uapi_headers
  BUILD_IN_SOURCE TRUE
  INSTALL_COMMAND ""
  STEP_TARGETS build
)
set(LIBBPF_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/libbpf)

find_package(PkgConfig)
pkg_check_modules(libbpf REQUIRED libelf)

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

add_dependencies(wasmedgePluginWasmBpf libbpf)
target_link_libraries(wasmedgePluginWasmBpf PUBLIC ${LIBBPF_LIBRARIES} -lelf -lz)
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

