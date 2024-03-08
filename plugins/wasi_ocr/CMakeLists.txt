# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

add_library(wasmedgePluginWasiOCR
  SHARED
  wasiocrenv.cpp
  wasiocrfunc.cpp
  wasiocrmodule.cpp
)

target_compile_options(wasmedgePluginWasiOCR
  PUBLIC
  -DWASMEDGE_PLUGIN
)

target_include_directories(wasmedgePluginWasiOCR
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
)

if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgePluginWasiOCR
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgePluginWasiOCR
    PRIVATE
    wasmedge_shared
  )
endif()

install(TARGETS wasmedgePluginWasiOCR DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge)

message(STATUS "WASI-OCR: Build Tesseract backend for WASI-OCR")
find_package(PkgConfig REQUIRED)
pkg_search_module(TESSERACT REQUIRED tesseract)
pkg_search_module(LEPTONICA REQUIRED lept)

target_include_directories(wasmedgePluginWasiOCR
  PUBLIC
  ${TESSERACT_INCLUDE_DIRS}
  ${LEPTONICA_INCLUDE_DIRS}
)

target_link_libraries(wasmedgePluginWasiOCR
  PUBLIC
  ${TESSERACT_LIBRARIES}
  ${LEPTONICA_LIBRARIES}
)
