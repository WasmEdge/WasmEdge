# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC
macro(tensorflow_customize _target _flag)
  get_target_property(_target_cxx_flags ${_target} COMPILE_OPTIONS)

  if(_target_cxx_flags)
    list(REMOVE_ITEM _target_cxx_flags ${_flag})
    list(APPEND _target_cxx_flags -w)
    set_target_properties(${_target} PROPERTIES COMPILE_OPTIONS "${_target_cxx_flags}")
  endif()
endmacro()

wasmedge_add_executable(wasiNNTests
  wasi_nn.cpp
)

# Prepare the testing data for each backends.
foreach(BACKEND ${WASMEDGE_PLUGIN_WASI_NN_BACKEND})
  if(BACKEND MATCHES "OpenVINO")
    message(STATUS "Download ML artifacts to ${CMAKE_CURRENT_BINARY_DIR}/wasinn_openvino_fixtures")
    execute_process(
      COMMAND bash ${CMAKE_SOURCE_DIR}/utils/wasi-nn/download-openvino-fixtures.sh ${CMAKE_CURRENT_BINARY_DIR}/wasinn_openvino_fixtures
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

    add_definitions(-DWASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO)
  elseif(BACKEND MATCHES "PyTorch")
    message(STATUS "Download ML artifacts to ${CMAKE_CURRENT_BINARY_DIR}/wasinn_pytorch_fixtures")
    execute_process(
      COMMAND bash ${CMAKE_SOURCE_DIR}/utils/wasi-nn/download-pytorch-fixtures.sh ${CMAKE_CURRENT_BINARY_DIR}/wasinn_pytorch_fixtures
      RESULT_VARIABLE DOWNLOAD_ERROR
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    file(MD5 ${CMAKE_CURRENT_BINARY_DIR}/wasinn_pytorch_fixtures/mobilenet.pt CHECKSUM_WEIGHT)
    file(MD5 ${CMAKE_CURRENT_BINARY_DIR}/wasinn_pytorch_fixtures/image-1x3x224x224.rgb CHECKSUM_IMAGE)

    if(NOT CHECKSUM_WEIGHT STREQUAL "234f446d2446e0f6fd8ed700c0b4b63b")
      message(FATAL_ERROR "mobilenet.pt downloaded with wrong md5")
    endif()

    if(NOT CHECKSUM_IMAGE STREQUAL "551caa6f3b66c1d953655228462570a1")
      message(FATAL_ERROR "image-1x3x224x224.rgb downloaded with wrong md5")
    endif()

    add_definitions(-DWASMEDGE_PLUGIN_WASI_NN_BACKEND_TORCH)
    find_package(Torch REQUIRED)
  elseif(BACKEND STREQUAL "Tensorflowlite")
    message(STATUS "Download ML artifacts to ${CMAKE_CURRENT_BINARY_DIR}/wasinn_tflite_fixtures")
    execute_process(
      COMMAND bash ${CMAKE_SOURCE_DIR}/utils/wasi-nn/download-tflite-fixtures.sh ${CMAKE_CURRENT_BINARY_DIR}/wasinn_tflite_fixtures
      RESULT_VARIABLE DOWNLOAD_ERROR
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    file(MD5 ${CMAKE_CURRENT_BINARY_DIR}/wasinn_tflite_fixtures/lite-model_aiy_vision_classifier_birds_V1_3.tflite CHECKSUM_WEIGHT)
    file(MD5 ${CMAKE_CURRENT_BINARY_DIR}/wasinn_tflite_fixtures/birdx224x224x3.rgb CHECKSUM_IMAGE)

    if(NOT CHECKSUM_WEIGHT STREQUAL "3e59cc3a99afeeb819c2c38b319a7938")
      message(FATAL_ERROR "downloaded tflite model with wrong md5")
    endif()

    if(NOT CHECKSUM_IMAGE STREQUAL "ad51c39cfe35d2ef35c4052b78cb3c55")
      message(FATAL_ERROR "downloaded bird.jpg fixture with wrong md5")
    endif()

    add_definitions(-DWASMEDGE_PLUGIN_WASI_NN_BACKEND_TFLITE)
    target_include_directories(wasiNNTests
      PUBLIC
      ${TENSORFLOW_INCLUDE}
    )
  elseif(BACKEND STREQUAL "Tensorflow")
    # -----------------------------------------------------------
    message(STATUS "Download ML artifacts to ${CMAKE_CURRENT_BINARY_DIR}/wasinn_tensorflow_fixtures")
    execute_process(
      COMMAND bash ${CMAKE_SOURCE_DIR}/utils/wasi-nn/download-tensorflow-fixtures.sh ${CMAKE_CURRENT_BINARY_DIR}/wasinn_tensorflow_fixtures
      RESULT_VARIABLE DOWNLOAD_ERROR
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    file(MD5 ${CMAKE_CURRENT_BINARY_DIR}/wasinn_tensorflow_fixtures/saved_model.pb CHECKSUM_WEIGHT)
    file(MD5 ${CMAKE_CURRENT_BINARY_DIR}/wasinn_tensorflow_fixtures/birdX224X224X3.rgb CHECKSUM_IMAGE)

    if(NOT CHECKSUM_WEIGHT STREQUAL "1f8786a5918f9252f6567743f9247759")
      message(FATAL_ERROR "downloaded tensorflow model with wrong md5")
    endif()

    if(NOT CHECKSUM_IMAGE STREQUAL "3adda9b92a0c01bb7f17fe5b34b7649f")
      message(FATAL_ERROR "downloaded image fixture with wrong md5")
    endif()

    add_definitions(-DWASMEDGE_PLUGIN_WASI_NN_BACKEND_TF)
    add_definitions(-DWASMEDGE_PLUGIN_WASI_NN_BACKEND_TF)
    target_include_directories(wasiNNTests
      PUBLIC
      ${TENSORFLOW_INCLUDE}
    )

    tensorflow_customize(wasiNNTests -Werror)
  else()
    # Add the other backend test files fetching here.
  endif()
endforeach()

target_link_libraries(wasiNNTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgePlugin
  wasmedgePluginWasiNN
)

add_test(wasiNNTests wasiNNTests)
