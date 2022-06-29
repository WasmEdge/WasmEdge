# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

wasmedge_add_executable(wasiNNTests
  wasi_nn.cpp
)

# Prepare the testing data for each backends.
foreach(BACKEND ${WASMEDGE_WASINN_BACKEND})
  if(BACKEND MATCHES "OpenVINO")
    message( STATUS "Download ML artifacts to ${CMAKE_CURRENT_BINARY_DIR}/wasinn_openvino_fixtures") 
    execute_process(
      COMMAND bash ${CMAKE_SOURCE_DIR}/utils/wasi-nn/download_openvino_fixtures.sh ${CMAKE_CURRENT_BINARY_DIR}/wasinn_openvino_fixtures
      RESULT_VARIABLE DOWNLOAD_ERROR
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    file(MD5 ${CMAKE_CURRENT_BINARY_DIR}/wasinn_openvino_fixtures/mobilenet.bin CHECKSUM_WEIGHT)
    file(MD5 ${CMAKE_CURRENT_BINARY_DIR}/wasinn_openvino_fixtures/mobilenet.xml CHECKSUM_DESCRIP)
    file(MD5 ${CMAKE_CURRENT_BINARY_DIR}/wasinn_openvino_fixtures/tensor-1x224x224x3-f32.bgr CHECKSUM_TENSOR)
    if(NOT CHECKSUM_WEIGHT STREQUAL "ae096b1f735f1e8e54bac8b2a42303bd")
      message(FATAL_ERROR "mobilenet.bin downloaded with wrong md5") 
    endif()
    if(NOT CHECKSUM_DESCRIP STREQUAL "4ea3a14273587ce5c1662018878f9f90")
      message(FATAL_ERROR "mobilenet.xml downloaded with wrong md5") 
    endif()
    if(NOT CHECKSUM_TENSOR STREQUAL "bfca546f4a3b5e6da49b7bd728e2799a")
      message(FATAL_ERROR "tensor-1x224x224x3-f32.bgr downloaded with wrong md5") 
    endif()
    add_definitions(-DWASMEDGE_WASINN_BACKEND_OPENVINO)
  else()
    # Add the other backend test files fetching here.
  endif()
endforeach()

target_link_libraries(wasiNNTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeHostModuleWasiNN
)

add_test(wasiNNTests wasiNNTests)
