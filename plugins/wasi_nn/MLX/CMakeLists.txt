wasmedge_add_library(
  mlx_cpp
  prompt/prompt.cpp
  model/transformer.cpp
  model/converter.cpp
  model/utils.cpp
  model/registry.cpp
  mlx/base.cpp
  mlx/linear.cpp
  mlx/positional_encoding.cpp
  mlx/activations.cpp
  mlx/embedding.cpp
  mlx/normalization.cpp
  mlx/transformer.cpp)

target_link_libraries(mlx_cpp PUBLIC mlx)
target_include_directories(mlx_cpp SYSTEM PUBLIC ./mlx)
target_include_directories(mlx_cpp SYSTEM PUBLIC ${MLX_INCLUDE_DIRS})
target_link_libraries(mlx_cpp PUBLIC ${MLX_LIBRARIES})

message(STATUS "Downloading gguflib")
FetchContent_Declare(
  gguflib
  GIT_REPOSITORY https://github.com/antirez/gguf-tools/
  GIT_TAG af7d88d808a7608a33723fba067036202910acb3
  GIT_SHALLOW FALSE
)
FetchContent_MakeAvailable(gguflib)
target_include_directories(mlx_cpp SYSTEM PUBLIC $<BUILD_INTERFACE:${gguflib_SOURCE_DIR}>)
add_library(gguflib STATIC ${gguflib_SOURCE_DIR}/fp16.c
                           ${gguflib_SOURCE_DIR}/gguflib.c)
set_target_properties(gguflib PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(mlx_cpp PUBLIC gguflib)
