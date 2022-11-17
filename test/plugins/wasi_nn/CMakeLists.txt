# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

wasmedge_add_executable(wasiNNTests
  wasi_nn.cpp
)

function(DOWNLOAD_FILE_FIXTRUE url filename dstdir hashmd5)
  message(STATUS "Download ${filename} ...")
  file(
    DOWNLOAD ${url}/${filename} 
    ${CMAKE_CURRENT_BINARY_DIR}/${dstdir}/${filename}
    EXPECTED_MD5 ${hashmd5}
    STATUS DOWNLOAD_RESULT
  )
  list(GET DOWNLOAD_RESULT 0 DOWNLOAD_RESULT_CODE)
    if(NOT DOWNLOAD_RESULT_CODE EQUAL 0)
        message(FATAL_ERROR "Failed downloading! Error: ${DOWNLOAD_RESULT}. The files can be downloaded by script in  ${CMAKE_SOURCE_DIR}/utils/wasi-nn}")
  endif()
endfunction()

# Prepare the testing data for each backends.
foreach(BACKEND ${WASMEDGE_PLUGIN_WASI_NN_BACKEND})
  if(BACKEND MATCHES "OpenVINO")
    set(openvino_fixtures_url "https://github.com/intel/openvino-rs/raw/v0.3.3/crates/openvino/tests/fixtures/mobilenet")
    message(STATUS "the download dir is ${CMAKE_CURRENT_BINARY_DIR}/wasinn_openvino_fixtures")  
    DOWNLOAD_FILE_FIXTRUE(${openvino_fixtures_url} "mobilenet.xml" "wasinn_openvino_fixtures" "4ea3a14273587ce5c1662018878f9f90")
    DOWNLOAD_FILE_FIXTRUE(${openvino_fixtures_url} "mobilenet.bin" "wasinn_openvino_fixtures" "ae096b1f735f1e8e54bac8b2a42303bd")
    DOWNLOAD_FILE_FIXTRUE(${openvino_fixtures_url} "tensor-1x224x224x3-f32.bgr" "wasinn_openvino_fixtures" "bfca546f4a3b5e6da49b7bd728e2799a")

    add_definitions(-DWASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO)
    find_package(InferenceEngine REQUIRED)
    target_link_libraries(wasiNNTests
      PUBLIC
      ${InferenceEngine_LIBRARIES}
    )

  elseif(BACKEND MATCHES "PyTorch")
    message( STATUS "Download ML artifacts to ${CMAKE_CURRENT_BINARY_DIR}/wasinn_pytorch_fixtures") 
    set(pytorch_fixtures_url "https://github.com/second-state/WasmEdge-WASINN-examples/raw/master/pytorch-mobilenet-image")
    DOWNLOAD_FILE_FIXTRUE(${pytorch_fixtures_url} "mobilenet.pt" "wasinn_pytorch_fixtures" "234f446d2446e0f6fd8ed700c0b4b63b")
    DOWNLOAD_FILE_FIXTRUE(${pytorch_fixtures_url} "image-1x3x224x224.rgb" "wasinn_pytorch_fixtures" "551caa6f3b66c1d953655228462570a1")

    add_definitions(-DWASMEDGE_PLUGIN_WASI_NN_BACKEND_TORCH)
    find_package(Torch REQUIRED)
    
  elseif(BACKEND STREQUAL "Tensorflowlite")
    message( STATUS "Download ML artifacts to ${CMAKE_CURRENT_BINARY_DIR}/wasinn_tflite_fixtures") 
    set(tflite_fixtures_url "https://raw.githubusercontent.com/gusye1234/WasmEdge-WASINN-examples/demo-tflite-image/tflite-birds_v1-image")
    DOWNLOAD_FILE_FIXTRUE(${tflite_fixtures_url} "lite-model_aiy_vision_classifier_birds_V1_3.tflite" "wasinn_tflite_fixtures" "3e59cc3a99afeeb819c2c38b319a7938")
    DOWNLOAD_FILE_FIXTRUE(${tflite_fixtures_url} "birdx224x224x3.rgb" "wasinn_tflite_fixtures" "ad51c39cfe35d2ef35c4052b78cb3c55")

    add_definitions(-DWASMEDGE_PLUGIN_WASI_NN_BACKEND_TFLITE)
    target_include_directories(wasiNNTests
      PUBLIC
      ${TENSORFLOW_INCLUDE}
    )
  else()
    # Add the other backend test files fetching here.
  endif()
endforeach()

target_link_libraries(wasiNNTests
  PUBLIC
  ${GTEST_BOTH_LIBRARIES}
  wasmedgePlugin
  wasmedgePluginWasiNN
)

add_test(wasiNNTests wasiNNTests)
