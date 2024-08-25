wasmedge_add_executable(wasmedgeStableDiffusionTests
  wasmedge_stablediffusion.cpp
)

add_dependencies(wasmedgeStableDiffusionTests
	wasmedgePluginWasmEdgeStableDiffusion
)

target_include_directories(wasmedgeStableDiffusionTests
	PUBLIC
	$<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:wasmedgePluginWasmEdgeStableDiffusion,INCLUDE_DIRECTORIES>
)

target_link_libraries(wasmedgeStableDiffusionTests
  PRIVATE
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
function(download URL OUTPUT HASH)
  file(DOWNLOAD
    ${URL}
    ${OUTPUT}
    SHOW_PROGRESS
    EXPECTED_HASH ${HASH}
  )
endfunction()
message(STATUS "Download ML artifacts to ${CMAKE_CURRENT_BINARY_DIR}/sd-v1-4.ckpt")
download(
  https://huggingface.co/CompVis/stable-diffusion-v-1-4-original/resolve/main/sd-v1-4.ckpt
  ${CMAKE_CURRENT_BINARY_DIR}/stableDiffusion/sd-v1-4.ckpt
  MD5=c01059060130b8242849d86e97212c84
)

add_test(wasmedgeStableDiffusionTests wasmedgeStableDiffusionTests)
