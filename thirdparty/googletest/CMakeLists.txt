# pthread is required by googletest
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(utilGoogleTest
  src/gtest-all.cc
)

target_link_libraries(utilGoogleTest
  Threads::Threads
)

target_include_directories(utilGoogleTest
  PUBLIC
  ${PROJECT_SOURCE_DIR}/thirdparty/googletest/include
  ${PROJECT_SOURCE_DIR}/thirdparty/googletest/
)
