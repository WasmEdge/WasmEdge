add_library(utilGoogleTest
  googletest/src/gtest-all.cc
)
target_link_libraries(utilGoogleTest Threads::Threads)
