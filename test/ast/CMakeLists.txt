# SPDX-License-Identifier: Apache-2.0

add_executable(ssvmASTTests
  moduleTest.cpp
  sectionTest.cpp
  descriptionTest.cpp
  segmentTest.cpp
  typeTest.cpp
  expressionTest.cpp
  instructionTest.cpp
)

target_link_libraries(ssvmASTTests
  PRIVATE
  utilGoogleTest
  ssvmLoaderFileMgr
  ssvmAST
)