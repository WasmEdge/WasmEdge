# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

message(STATUS "WASI-OCR: Build Tesseract backend for WASI-OCR")
find_package(PkgConfig REQUIRED)
pkg_search_module(TESSERACT REQUIRED tesseract)
pkg_search_module(LEPTONICA REQUIRED lept)

wasmedge_add_plugin(wasmedgePluginWasiOCR
  BUNDLE ${WASMEDGE_BUNDLE_PLUGIN_OCR}
  SOURCES
  wasiocrenv.cpp
  wasiocrfunc.cpp
  wasiocrmodule.cpp
)

target_include_directories(wasmedgePluginWasiOCR
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${TESSERACT_INCLUDE_DIRS}
  ${LEPTONICA_INCLUDE_DIRS}
)

target_link_libraries(wasmedgePluginWasiOCR
  PUBLIC
  ${TESSERACT_LIBRARIES}
  ${LEPTONICA_LIBRARIES}
)

install(
  TARGETS wasmedgePluginWasiOCR
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge
)
