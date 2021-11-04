# SPDX-License-Identifier: Apache-2.0

find_library(ONNXRUNTIME_LIBRARY
  onnxruntime
  HINTS
  ${PROJECT_SOURCE_DIR}/thirdparty/onnxruntime-linux-x64-1.9.0/lib
)

wasmedge_add_library(wasmedgeHostModuleWasiNN
  wasinnfunc.cpp
  wasinnmodule.cpp
)

target_include_directories(wasmedgeHostModuleWasiNN
  PUBLIC
  ${Boost_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
  ${PROJECT_SOURCE_DIR}/thirdparty/onnxruntime-linux-x64-1.9.0/include
  ${PROJECT_SOURCE_DIR}/thirdparty/onnxruntime-linux-x64-1.9.0/include/onnxruntime/core/session
)

target_link_libraries(wasmedgeHostModuleWasiNN
  PUBLIC
  wasmedgeSystem
  ${ONNXRUNTIME_LIBRARY}
)
