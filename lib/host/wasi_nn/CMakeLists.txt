# SPDX-License-Identifier: Apache-2.0

wasmedge_add_library(wasmedgeHostModuleWasiNN
  wasinnfunc.cpp
  wasinnmodule.cpp
)

list(APPEND INCLUDE_DIRS "${Boost_INCLUDE_DIRS}")
list(APPEND INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/include")
list(APPEND INCLUDES_DIRS "${PROJECT_SOURCE_DIR}/thirdparty")


list(APPEND LIB_DIRS "wasmedgeSystem")

# add backends building flags
# add_definitions(-DWASINN_BUILD_ONNX)
add_definitions(-DWASINN_BUILD_OPENVINO)

get_directory_property(WASINNDEFS COMPILE_DEFINITIONS)

if(WASINNDEFS MATCHES "WASINN_BUILD_ONNX")
  message( STATUS "Build ONNX backend for WASI-NN")
  find_library(ONNXRUNTIME_LIBRARY
    onnxruntime
    HINTS
    ${PROJECT_SOURCE_DIR}/thirdparty/onnxruntime-linux-x64-1.9.0/lib
  )
  list(APPEND INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/thirdparty/onnxruntime-linux-x64-1.9.0/include")
  list(APPEND INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/thirdparty/onnxruntime-linux-x64-1.9.0/include/onnxruntime/core/session")
  list(APPEND LIB_DIRS "${ONNXRUNTIME_LIBRARY}")
endif()

if(WASINNDEFS MATCHES "WASINN_BUILD_OPENVINO")
  message( STATUS "Build OpenVINO backend for WASI-NN")
  find_package(ngraph REQUIRED)
  find_package(InferenceEngine REQUIRED)
  list(APPEND LIB_DIRS ${InferenceEngine_LIBRARIES} )
  list(APPEND LIB_DIRS ${NGRAPH_LIBRARIES})
endif()


target_include_directories(wasmedgeHostModuleWasiNN
  PUBLIC
  ${INCLUDE_DIRS}
)

target_link_libraries(wasmedgeHostModuleWasiNN
  PUBLIC
  ${LIB_DIRS}
)
