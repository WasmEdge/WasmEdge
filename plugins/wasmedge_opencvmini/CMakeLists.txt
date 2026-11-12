# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

find_package(OpenCV 5 REQUIRED COMPONENTS core imgproc imgcodecs)

if(NOT OpenCV_VERSION_MAJOR EQUAL 5)
  message(FATAL_ERROR "OpenCVMini requires OpenCV major version 5")
endif()

message(STATUS "OpenCVMini: OpenCV version ${OpenCV_VERSION}")

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
)

target_include_directories(wasmedgePluginWasmEdgeOpenCVMini
  SYSTEM PUBLIC
  ${OpenCV_INCLUDE_DIRS}
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
  COMPONENT WasmEdge
)
