# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

message(STATUS "WasmEdge-OCR: Build for Tesseract backend.")
find_package(PkgConfig REQUIRED)
pkg_search_module(TESSERACT REQUIRED tesseract)
pkg_search_module(LEPTONICA REQUIRED lept)

wasmedge_add_plugin(wasmedgePluginWasmEdgeOCR
  BUNDLE ${WASMEDGE_BUNDLE_PLUGIN_OCR}
  SOURCES
  ocr_env.cpp
  ocr_func.cpp
  ocr_module.cpp
)

target_include_directories(wasmedgePluginWasmEdgeOCR
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${TESSERACT_INCLUDE_DIRS}
  ${LEPTONICA_INCLUDE_DIRS}
)

target_link_libraries(wasmedgePluginWasmEdgeOCR
  PUBLIC
  ${TESSERACT_LIBRARIES}
  ${LEPTONICA_LIBRARIES}
)

install(
  TARGETS wasmedgePluginWasmEdgeOCR
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge
)
