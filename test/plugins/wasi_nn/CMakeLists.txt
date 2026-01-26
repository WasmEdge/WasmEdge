# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

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
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "s390x")
      # Use a big endian model for s390x
      download(
      https://huggingface.co/taronaeo/Granite-3.0-1B-A400M-Instruct-BE-GGUF/resolve/main/granite-3.0-1b-a400m-instruct-be.Q2_K.gguf
      ${CMAKE_CURRENT_BINARY_DIR}/wasinn_ggml_fixtures/granite-3.gguf
      MD5=2520fd8a702468942fdd1595b9dca9a2
      )
    else()
      download(
      https://huggingface.co/TheBloke/orca_mini_v3_7B-GGUF/resolve/main/orca_mini_v3_7b.Q2_K.gguf
      ${CMAKE_CURRENT_BINARY_DIR}/wasinn_ggml_fixtures/orca_mini.gguf
      MD5=f895f00678bfbf89f70d6d25f20a7b5f
      )
    endif()
    if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
      target_compile_options(wasiNNTests PUBLIC
        /wd4067 # unexpected tokens following preprocessor directive - expected a newline
        /wd4505 # unreferenced local function has been removed
      )
    else()
      # string_split<std::string> in common.h unused
      target_compile_options(wasiNNTests PUBLIC
        -Wno-unused-function
      )
    endif()
  elseif(BACKEND STREQUAL "piper")
    message(STATUS "Download ML artifacts to ${CMAKE_CURRENT_BINARY_DIR}/wasinn_piper_fixtures")
    download(
      https://github.com/OHF-Voice/piper1-gpl/raw/v1.3.0/tests/test_voice.onnx
      ${CMAKE_CURRENT_BINARY_DIR}/wasinn_piper_fixtures/test_voice.onnx
      SHA256=1c8bbb420741358f0a356bb83eaae1b4161fbb5974f6941e10eb5a1725d78994
    )
    download(
      https://github.com/OHF-Voice/piper1-gpl/raw/v1.3.0/tests/test_voice.onnx.json
      ${CMAKE_CURRENT_BINARY_DIR}/wasinn_piper_fixtures/test_voice.onnx.json
      SHA256=ccd28e02c334fbcfc94a86c8f86f1d7dbb5bffc844af9f22243a1f9f7840db1b
    )
    set(ESPEAK_SOURCE_DIR "")

    if (DEFINED PIPER_ROOT)
        set(ESPEAK_SOURCE_DIR "${PIPER_ROOT}/espeak-ng-data")
    elseif(EXISTS "${CMAKE_BINARY_DIR}/espeak_ng-install/share/espeak-ng-data")
        set(ESPEAK_SOURCE_DIR "${CMAKE_BINARY_DIR}/espeak_ng-install/share/espeak-ng-data")
    endif()

    if (EXISTS "${ESPEAK_SOURCE_DIR}")
        file(
          COPY ${ESPEAK_SOURCE_DIR}
          DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/wasinn_piper_fixtures
        )
    else()
        message(WARNING "Could not find espeak-ng-data at ${ESPEAK_SOURCE_DIR}")
    endif()
    if(DEFINED PIPER_ROOT)
      find_library(ESPEAK_NG_LIB
          NAMES espeak-ng libespeak-ng
          PATHS /usr/local/lib /usr/local/lib64
          NO_DEFAULT_PATH
      )
      if (NOT ESPEAK_NG_LIB)
          find_library(ESPEAK_NG_LIB NAMES espeak-ng libespeak-ng)
      endif()
      find_library(UCD_LIB
          NAMES ucd libucd
          PATHS /usr/local/lib /usr/local/lib64
          NO_DEFAULT_PATH
      )
      if (NOT UCD_LIB)
          find_library(UCD_LIB NAMES ucd libucd)
      endif()
      set(ESPEAK_TARGETS ${ESPEAK_NG_LIB} ${UCD_LIB})

    else()
      set(ESPEAK_TARGETS "")
    endif()

    target_link_libraries(wasiNNTests
      PRIVATE
      onnxruntime
      ${ESPEAK_TARGETS}
    )
  elseif(BACKEND STREQUAL "whisper")
    message( STATUS "Download ML artifacts to ${CMAKE_CURRENT_BINARY_DIR}/wasinn_whisper_fixtures")
    download(
      https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin
      ${CMAKE_CURRENT_BINARY_DIR}/wasinn_whisper_fixtures/ggml-base.bin
      MD5=4279db3d7b18d9f6e4d5817a16af4f09
    )
    download(
      https://github.com/second-state/WasmEdge-WASINN-examples/raw/master/wasmedge-ggml/whisper/test.wav
      ${CMAKE_CURRENT_BINARY_DIR}/wasinn_whisper_fixtures/test.wav
      MD5=6cf3f7af1ebbd6b29c373e526b548dba
    )
  elseif(BACKEND STREQUAL "mlx")
    message( STATUS "Download ML artifacts to ${CMAKE_CURRENT_BINARY_DIR}/wasinn_mlx_fixtures")
    download(
      https://huggingface.co/TinyLlama/TinyLlama-1.1B-Chat-v1.0/resolve/main/model.safetensors
      ${CMAKE_CURRENT_BINARY_DIR}/wasinn_mlx_fixtures/model.safetensors
      MD5=59e1605b3af5f1673eb8396251d6bc46
    )
    download(
      https://huggingface.co/TinyLlama/TinyLlama-1.1B-Chat-v1.0/resolve/main/tokenizer.json
      ${CMAKE_CURRENT_BINARY_DIR}/wasinn_mlx_fixtures/tokenizer.json
      MD5=c9dc953a24ad2b76b4bae4bf456f18bd
    )
    target_compile_options(wasiNNTests PUBLIC
      -Wno-unused-parameter
    )

  elseif(BACKEND STREQUAL "bitnet")
    message(STATUS "Download ML artifacts to ${CMAKE_CURRENT_BINARY_DIR}/wasinn_bitnet_fixtures")
    download(
      https://huggingface.co/microsoft/bitnet-b1.58-2B-4T-gguf/resolve/main/ggml-model-i2_s.gguf
      ${CMAKE_CURRENT_BINARY_DIR}/wasinn_bitnet_fixtures/ggml-model-i2_s.gguf
      MD5=65cb04366e4d02ccd78b4b7b48c84b3b
    )
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
      target_compile_options(wasiNNTests PUBLIC
        -Wno-unused-function
      )
    endif()
  elseif(BACKEND STREQUAL "rwkv")
    message(STATUS "Download ML artifacts to ${CMAKE_CURRENT_BINARY_DIR}/wasinn_rwkv_fixtures")
    download(
      https://huggingface.co/concedo/rwkv-v4-169m-ggml/resolve/main/rwkv-169m-ggml-f16-new.bin
      ${CMAKE_CURRENT_BINARY_DIR}/wasinn_rwkv_fixtures/rwkv-4-pile-169m.bin
      SHA256=97c1fbc15ba61b968fb74bb93ea75f84ee98514e9e76552436ccb9b5f3a2a2af
    )
    download(
      https://raw.githubusercontent.com/BlinkDL/ChatRWKV/main/20B_tokenizer.json
      ${CMAKE_CURRENT_BINARY_DIR}/wasinn_rwkv_fixtures/20B_tokenizer.json
      MD5=55634e54c40440d3639fc80ef8e7bdd9
    )

    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
      target_compile_options(wasiNNTests PUBLIC
        -Wno-unused-function
      )
    endif()
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
