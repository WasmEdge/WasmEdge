add_executable(ssvmLoaderFileMgrTests
  filemgrTest.cpp
)

add_executable(ssvmLoaderASTTests
  moduleTest.cpp
  sectionTest.cpp
  descriptionTest.cpp
  segmentTest.cpp
  typeTest.cpp
  expressionTest.cpp
  instructionTest.cpp
)

target_link_libraries(ssvmLoaderFileMgrTests
  PRIVATE
  utilGoogleTest
  ssvmLoader
)

target_link_libraries(ssvmLoaderASTTests
  PRIVATE
  ssvmLoader
  gtest
)