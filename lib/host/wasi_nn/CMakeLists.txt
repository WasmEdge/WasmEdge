# SPDX-License-Identifier: Apache-2.0

wasmedge_add_library(wasmedgeHostModuleWasiNN
  wasinnfunc.cpp
  wasinnmodule.cpp
)

# add backends building flags
if(WASMEDGE_WASINN_BUILD_OPENVINO)
  message( STATUS "Build OpenVINO backend for WASI-NN")
  add_definitions(-DWASMEDGE_WASINN_BUILD_OPENVINO)
  find_package(InferenceEngine REQUIRED)
  target_link_libraries(wasmedgeHostModuleWasiNN
    PUBLIC
    ${InferenceEngine_LIBRARIES}
  )
endif()

target_link_libraries(wasmedgeHostModuleWasiNN
  PUBLIC
  wasmedgeCommon
  wasmedgeSystem
)
