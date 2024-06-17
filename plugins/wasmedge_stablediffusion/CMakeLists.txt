 # setup stable diffusion
message(STATUS "Downloading stable diffusion source")
if (MSVC)
  add_compile_options(
    /wd4996
    /wd4456
    /wd4459
    /wd4100
    /wd4127
    /wd4701
  )
else()
	add_compile_options(
		-Wno-unused-function
		-Wno-unused-variable
		-Wno-unused-parameter
		-Wno-missing-field-initializers
	)
endif()
FetchContent_Declare(
  stable-diffusion
  GIT_REPOSITORY https://github.com/leejet/stable-diffusion.cpp.git
  GIT_TAG        master-9c51d87
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(stable-diffusion)
set_property(TARGET stable-diffusion PROPERTY POSITION_INDEPENDENT_CODE ON)
get_target_property(SD_DEPS stable-diffusion LINK_LIBRARIES)
foreach(dep ${SD_DEPS})
if(TARGET ${dep})
    set_target_properties(${dep} PROPERTIES
      POSITION_INDEPENDENT_CODE ON
    )
  endif()
endforeach()

wasmedge_add_library(wasmedgePluginStableDiffusion
  SHARED
  sd_env.cpp
  sd_func.cpp
  sd_module.cpp
)

target_link_libraries(wasmedgePluginStableDiffusion PRIVATE stable-diffusion ${CMAKE_THREAD_LIBS_INIT})

target_compile_options(wasmedgePluginStableDiffusion
  PUBLIC
  -DWASMEDGE_PLUGIN
)

if(WASMEDGE_LINK_PLUGINS_STATIC)
target_link_libraries(wasmedgePluginStableDiffusion
  PRIVATE
  wasmedgeCAPI
)

else()
target_link_libraries(wasmedgePluginStableDiffusion
  PRIVATE
  wasmedge_shared
)
endif()

target_include_directories(wasmedgePluginStableDiffusion
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
)

install(TARGETS wasmedgePluginStableDiffusion DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge)