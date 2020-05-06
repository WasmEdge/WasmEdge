# SPDX-License-Identifier: Apache-2.0

add_library(ssvmHostFuncONNC
  runtime_add_float.cpp
  runtime_add_int8.cpp
  runtime_averagepool_float.cpp
  runtime_batchnormalization_float.cpp
  runtime_batchnormalization_int8.cpp
  runtime_concat_float.cpp
  runtime_conv_float.cpp
  runtime_conv_int8.cpp
  runtime_gemm_float.cpp
  runtime_globalaveragepool_float.cpp
  runtime_lrn_float.cpp
  runtime_maxpool_float.cpp
  runtime_maxpool_int8.cpp
  runtime_mul_float.cpp
  runtime_mul_int8.cpp
  runtime_relu_float.cpp
  runtime_relu_int8.cpp
  runtime_reshape_float.cpp
  runtime_softmax_float.cpp
  runtime_sum_float.cpp
  runtime_transpose_float.cpp
  runtime_unsqueeze_float.cpp
)

if(ONNC_WASM_LIBRARY)
  target_link_libraries(ssvmHostFuncONNC
    PRIVATE
    ${ONNC_WASM_LIBRARY}
  )
endif()

target_include_directories(ssvmHostFuncONNC
  PUBLIC
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
  ${PROJECT_SOURCE_DIR}/thirdparty/evmc/include
)
