# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

find_package(OpenCV 4 REQUIRED)

wasmedge_add_plugin(wasmedgePluginWasmEdgeOpenCVMini
  BUNDLE ${WASMEDGE_BUNDLE_PLUGIN_OPENCVMINI}
  SOURCES
  opencvmini_env.cpp
  opencvmini_func.cpp
  opencvmini_module.cpp
)

target_include_directories(wasmedgePluginWasmEdgeOpenCVMini
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${OpenCV_INClUDE_DIRS}
)

target_link_libraries(wasmedgePluginWasmEdgeOpenCVMini
  PRIVATE
  ${OpenCV_LIBS}
)

install(
  TARGETS wasmedgePluginWasmEdgeOpenCVMini
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge
)
