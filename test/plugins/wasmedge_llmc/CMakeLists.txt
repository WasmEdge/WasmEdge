# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_executable(wasmedgeLLMCTests
  wasmedge_llmc.cpp
)

add_dependencies(wasmedgeLLMCTests
  wasmedgePluginWasmEdgeLLMC
)

target_include_directories(wasmedgeLLMCTests
	PUBLIC
	$<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:wasmedgePluginWasmEdgeLLMC,INCLUDE_DIRECTORIES>
)

target_link_libraries(wasmedgeLLMCTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
)

# Link to the WasmEdge library
if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgeLLMCTests
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgeLLMCTests
    PRIVATE
    wasmedge_shared
  )
endif()

function(download URL OUTPUT HASH)
  file(DOWNLOAD
    ${URL}
    ${OUTPUT}
    SHOW_PROGRESS
    EXPECTED_HASH ${HASH}
  )
endfunction()

message(STATUS "Downloading GPT2 model check point to ${CMAKE_CURRENT_BINARY_DIR}/gpt2_124M.bin")
if (WASMEDGE_PLUGIN_LLMC_CUDA)
  download(
    https://huggingface.co/datasets/karpathy/llmc-starter-pack/resolve/main/gpt2_124M_bf16.bin
    ${CMAKE_CURRENT_BINARY_DIR}/wasmedge_llmc/gpt2_124M.bin
    SHA256=6661f45628102b4c6e86835d9057b5ba2c024dbf9b81445175e258b7878a1a6f
  )
else()
  download(
    https://huggingface.co/datasets/karpathy/llmc-starter-pack/resolve/main/gpt2_124M.bin
    ${CMAKE_CURRENT_BINARY_DIR}/wasmedge_llmc/gpt2_124M.bin
    SHA256=3da8b207584030bcdcd207cf7a99952e3421dce92da218b351071857511bf162
  )
endif()
message(STATUS "Downloading training dataset to ${CMAKE_CURRENT_BINARY_DIR}/tiny_shakespeare_train.bin")
download(
  https://huggingface.co/datasets/karpathy/llmc-starter-pack/resolve/main/tiny_shakespeare_train.bin
  ${CMAKE_CURRENT_BINARY_DIR}/wasmedge_llmc/tiny_shakespeare_train.bin
  SHA256=8a70606be574040c26d225694f5f9759973b419852d22f7fe5c118e1b359dcc8
)
message(STATUS "Downloading validation dataset to ${CMAKE_CURRENT_BINARY_DIR}/tiny_shakespeare_val.bin")
download(
  https://huggingface.co/datasets/karpathy/llmc-starter-pack/resolve/main/tiny_shakespeare_val.bin
  ${CMAKE_CURRENT_BINARY_DIR}/wasmedge_llmc/tiny_shakespeare_val.bin
  SHA256=fe99db720dc7c83e694806d4e047a952909411da1daccde4ccc2e55f40882a62
)
message(STATUS "Downloading tokenizer data to ${CMAKE_CURRENT_BINARY_DIR}/gpt2_tokenizer.bin")
download(
  https://huggingface.co/datasets/karpathy/llmc-starter-pack/resolve/main/gpt2_tokenizer.bin
  ${CMAKE_CURRENT_BINARY_DIR}/wasmedge_llmc/gpt2_tokenizer.bin
  SHA256=6f3abc21e444e4e8300e225f4e03da48ea121cf17e30f67009b8dad7a66c2f13
)

add_test(wasmedgeLLMCTests wasmedgeLLMCTests)
