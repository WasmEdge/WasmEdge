# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_executable(wasmedgeLoaderFileMgrTests
  filemgrTest.cpp
)

add_test(wasmedgeLoaderFileMgrTests wasmedgeLoaderFileMgrTests)

file(COPY
  ${CMAKE_CURRENT_SOURCE_DIR}/filemgrTestData
  DESTINATION
  ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(wasmedgeLoaderFileMgrTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeLoaderFileMgr
)

wasmedge_add_executable(wasmedgeLoaderASTTests
  moduleTest.cpp
  sectionTest.cpp
  descriptionTest.cpp
  segmentTest.cpp
  typeTest.cpp
  expressionTest.cpp
  instructionTest.cpp
)

add_test(wasmedgeLoaderASTTests wasmedgeLoaderASTTests)

target_link_libraries(wasmedgeLoaderASTTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeLoader
)

wasmedge_add_executable(wasmedgeLoaderSerializerTests
  serializeModuleTest.cpp
  serializeSectionTest.cpp
  serializeDescriptionTest.cpp
  serializeSegmentTest.cpp
  serializeTypeTest.cpp
  serializeExpressionTest.cpp
  serializeInstructionTest.cpp
)

add_test(wasmedgeLoaderSerializerTests wasmedgeLoaderSerializerTests)

target_link_libraries(wasmedgeLoaderSerializerTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeLoader
)
