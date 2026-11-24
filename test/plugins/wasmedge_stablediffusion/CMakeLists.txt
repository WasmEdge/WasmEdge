wasmedge_add_executable(wasmedgeStableDiffusionTests
  wasmedge_stablediffusion.cpp
  sd_abi_test.cpp
  sd_adapter_check_test.cpp
  sd_image_test.cpp
  sd_request_test.cpp
)

add_dependencies(wasmedgeStableDiffusionTests
	wasmedgePluginWasmEdgeStableDiffusion
)

target_include_directories(wasmedgeStableDiffusionTests
	PUBLIC
	$<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:wasmedgePluginWasmEdgeStableDiffusion,INCLUDE_DIRECTORIES>
)

FetchContent_GetProperties(stable-diffusion)
target_include_directories(wasmedgeStableDiffusionTests
  SYSTEM
  PRIVATE
  "${stable-diffusion_SOURCE_DIR}/thirdparty"
)

target_link_libraries(wasmedgeStableDiffusionTests
  PRIVATE
  wasmedgePluginWasmEdgeStableDiffusionCore
  ${GTEST_BOTH_LIBRARIES}
)

# Link to the WasmEdge library
if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgeStableDiffusionTests
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgeStableDiffusionTests
    PRIVATE
    wasmedge_shared
  )
endif()
message(STATUS "Download ML artifacts to ${CMAKE_CURRENT_BINARY_DIR}/stableDiffusion/sd-v1-5-fp16.safetensors")
wasmedge_download(
  https://huggingface.co/Comfy-Org/stable-diffusion-v1-5-archive/resolve/main/v1-5-pruned-emaonly-fp16.safetensors
  ${CMAKE_CURRENT_BINARY_DIR}/stableDiffusion/sd-v1-5-fp16.safetensors
  SHA256=e9476a13728cd75d8279f6ec8bad753a66a1957ca375a1464dc63b37db6e3916
)

add_test(wasmedgeStableDiffusionTests wasmedgeStableDiffusionTests)
