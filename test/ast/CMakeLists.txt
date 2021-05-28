# SPDX-License-Identifier: Apache-2.0

add_executable(wasmedgeASTTests
  moduleTest.cpp
  sectionTest.cpp
  descriptionTest.cpp
  segmentTest.cpp
  typeTest.cpp
  expressionTest.cpp
  instructionTest.cpp
)

add_test(wasmedgeASTTests wasmedgeASTTests)

target_link_libraries(wasmedgeASTTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeLoaderFileMgr
  wasmedgeAST
)
