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
  GIT_TAG        master-1d2af5c
  GIT_SHALLOW    FALSE
)
FetchContent_MakeAvailable(stable-diffusion)
set_property(TARGET stable-diffusion PROPERTY POSITION_INDEPENDENT_CODE ON)

wasmedge_add_library(wasmedgePluginStableDiffusion
  SHARED
  stablediffusion.cpp
)

install(TARGETS wasmedgePluginStableDiffusion RUNTIME)
target_link_libraries(wasmedgePluginStableDiffusion PRIVATE stable-diffusion ${CMAKE_THREAD_LIBS_INIT})