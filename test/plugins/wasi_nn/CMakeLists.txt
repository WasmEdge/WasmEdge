# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

wasmedge_add_executable(wasiNNTests
  wasi_nn.cpp
)

function(download URL OUTPUT HASH)
  file(DOWNLOAD
    ${URL}
    ${OUTPUT}
    SHOW_PROGRESS
    EXPECTED_HASH ${HASH}
  )
endfunction()

# Prepare the testing data for each backends.
foreach(BACKEND ${WASMEDGE_PLUGIN_WASI_NN_BACKEND})
  string(TOLOWER ${BACKEND} BACKEND)
  if(BACKEND MATCHES "openvino")
    message(STATUS "Download ML artifacts to ${CMAKE_CURRENT_BINARY_DIR}/wasinn_openvino_fixtures")
    download(
      https://github.com/intel/openvino-rs/raw/v0.3.3/crates/openvino/tests/fixtures/mobilenet/mobilenet.bin
      ${CMAKE_CURRENT_BINARY_DIR}/wasinn_openvino_fixtures/mobilenet.bin
      MD5=ae096b1f735f1e8e54bac8b2a42303bd
    )
    download(
      https://github.com/intel/openvino-rs/raw/v0.3.3/crates/openvino/tests/fixtures/mobilenet/mobilenet.xml
      ${CMAKE_CURRENT_BINARY_DIR}/wasinn_openvino_fixtures/mobilenet.xml
      MD5=4ea3a14273587ce5c1662018878f9f90
    )
    download(
      https://github.com/intel/openvino-rs/raw/v0.3.3/crates/openvino/tests/fixtures/mobilenet/tensor-1x224x224x3-f32.bgr
      ${CMAKE_CURRENT_BINARY_DIR}/wasinn_openvino_fixtures/tensor-1x224x224x3-f32.bgr
      MD5=bfca546f4a3b5e6da49b7bd728e2799a
    )
  elseif(BACKEND MATCHES "pytorch")
    message(STATUS "Download ML artifacts to ${CMAKE_CURRENT_BINARY_DIR}/wasinn_pytorch_fixtures")
    download(
      https://github.com/second-state/WasmEdge-WASINN-examples/raw/master/pytorch-mobilenet-image/mobilenet.pt
      ${CMAKE_CURRENT_BINARY_DIR}/wasinn_pytorch_fixtures/mobilenet.pt
      MD5=234f446d2446e0f6fd8ed700c0b4b63b
    )
    download(
      https://github.com/second-state/WasmEdge-WASINN-examples/raw/master/pytorch-mobilenet-image/image-1x3x224x224.rgb
      ${CMAKE_CURRENT_BINARY_DIR}/wasinn_pytorch_fixtures/image-1x3x224x224.rgb
      MD5=551caa6f3b66c1d953655228462570a1
    )
  elseif(BACKEND STREQUAL "tensorflowlite")
    message(STATUS "Download ML artifacts to ${CMAKE_CURRENT_BINARY_DIR}/wasinn_tflite_fixtures")
    download(
      https://raw.githubusercontent.com/gusye1234/WasmEdge-WASINN-examples/demo-tflite-image/tflite-birds_v1-image/lite-model_aiy_vision_classifier_birds_V1_3.tflite
      ${CMAKE_CURRENT_BINARY_DIR}/wasinn_tflite_fixtures/lite-model_aiy_vision_classifier_birds_V1_3.tflite
      MD5=3e59cc3a99afeeb819c2c38b319a7938
    )
    download(
      https://raw.githubusercontent.com/gusye1234/WasmEdge-WASINN-examples/demo-tflite-image/tflite-birds_v1-image/birdx224x224x3.rgb
      ${CMAKE_CURRENT_BINARY_DIR}/wasinn_tflite_fixtures/birdx224x224x3.rgb
      MD5=ad51c39cfe35d2ef35c4052b78cb3c55
    )
  elseif(BACKEND STREQUAL "ggml")
    message(STATUS "Download ML artifacts to ${CMAKE_CURRENT_BINARY_DIR}/wasinn_ggml_fixtures")
    download(
      https://huggingface.co/TheBloke/orca_mini_v3_7B-GGUF/resolve/main/orca_mini_v3_7b.Q2_K.gguf
      ${CMAKE_CURRENT_BINARY_DIR}/wasinn_ggml_fixtures/orca_mini.gguf
      MD5=f895f00678bfbf89f70d6d25f20a7b5f
    )
    if(MSVC)
      target_compile_options(wasiNNTests PUBLIC
        /wd4067 # unexpected tokens following preprocessor directive - expected a newline
      )
    endif()
  elseif(BACKEND STREQUAL "neuralspeed")
    message( STATUS "Download ML artifacts to ${CMAKE_CURRENT_BINARY_DIR}/wasinn_neural_speed_fixtures")
    download(
      https://huggingface.co/TheBloke/Llama-2-7B-Chat-GGUF/resolve/main/llama-2-7b-chat.Q4_0.gguf
      ${CMAKE_CURRENT_BINARY_DIR}/wasinn_neural_speed_fixtures/llama-2-7b-chat.Q4_0.gguf
      MD5=09ff9df730d4b5a8b60fcd89e97ee6f4
    )
  else()
    # Add the other backend test files fetching here.
  endif()
endforeach()

add_dependencies(wasiNNTests
  wasmedgePluginWasiNN
)

include(WASINNDeps)
wasmedge_setup_wasinn_target(wasiNNTests)

target_include_directories(wasiNNTests
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:wasmedgePluginWasiNN,INCLUDE_DIRECTORIES>
)

target_link_libraries(wasiNNTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
)
# Link to the WasmEdge library
if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasiNNTests
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasiNNTests
    PRIVATE
    wasmedge_shared
  )
endif()

add_test(wasiNNTests wasiNNTests)

if(WASMEDGE_BUILD_WASI_NN_RPC)
  add_definitions(-DWASMEDGE_BUILD_WASI_NN_RPC)
  target_link_libraries(wasiNNTests
    PRIVATE
    wasiNNRPC
  )
endif()
