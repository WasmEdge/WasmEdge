wasmedge_add_executable(wasmedgeStableDiffusionTests
  wasmedge_stablediffusion.cpp
)

add_dependencies(wasmedgeStableDiffusionTests
	wasmedgePluginStableDiffusion
)

target_include_directories(wasmedgeStableDiffusionTests
	PUBLIC
	$<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:wasmedgePluginStableDiffusion,INCLUDE_DIRECTORIES>
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

add_test(wasmedgeStableDiffusionTests wasmedgeStableDiffusionTests)