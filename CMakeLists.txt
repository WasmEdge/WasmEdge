cmake_minimum_required(VERSION 3.9)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

include_directories(BEFORE
  ${CMAKE_CURRENT_BINARY_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTS "Generate build targets for the ssvm unit tests." OFF)

add_subdirectory(include)
add_subdirectory(lib)
add_subdirectory(tools/ssvm)

if(BUILD_TESTS)
  include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/unittest/googletest/include
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/unittest/googletest
  )
  # pthread is required by googletest
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)

  add_subdirectory(test)
  add_subdirectory(utils/unittest)
endif()
