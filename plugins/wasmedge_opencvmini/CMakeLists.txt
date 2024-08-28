# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

find_package(OpenCV 4 REQUIRED)

wasmedge_add_library(wasmedgePluginWasmEdgeOpenCVMini
  SHARED
  opencvmini_env.cpp
  opencvmini_func.cpp
  opencvmini_module.cpp
)

target_compile_options(wasmedgePluginWasmEdgeOpenCVMini
  PUBLIC
  -DWASMEDGE_PLUGIN
)

target_include_directories(wasmedgePluginWasmEdgeOpenCVMini
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${OpenCV_INClUDE_DIRS}
)

if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgePluginWasmEdgeOpenCVMini
    PRIVATE
    wasmedgeCAPI
    ${OpenCV_LIBS}
  )
else()
  target_link_libraries(wasmedgePluginWasmEdgeOpenCVMini
    PRIVATE
    wasmedge_shared
    ${OpenCV_LIBS}
  )
endif()

install(
  TARGETS wasmedgePluginWasmEdgeOpenCVMini
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge
)
