cmake_minimum_required(VERSION 3.9)
project(SSVM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

include_directories(BEFORE
  ${CMAKE_CURRENT_BINARY_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_BINARY_DIR}/thirdparty/keccak
  ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/keccak
  ${Boost_INCLUDE_DIR}
)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTS "Generate build targets for the ssvm unit tests." OFF)

find_package(Boost REQUIRED)

if(BUILD_TESTS)
  include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}/thirdparty/googletest/include
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/googletest/include
    ${CMAKE_CURRENT_BINARY_DIR}/thirdparty/googletest/
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/googletest/
  )
  add_subdirectory(test)
endif()

add_subdirectory(include)
add_subdirectory(lib)
add_subdirectory(thirdparty)
add_subdirectory(tools/ssvm)