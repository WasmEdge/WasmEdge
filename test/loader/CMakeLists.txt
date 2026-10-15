# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

file(COPY
  ${CMAKE_CURRENT_SOURCE_DIR}/filemgrTestData
  DESTINATION
  ${CMAKE_CURRENT_BINARY_DIR}
)

wasmedge_add_executable(wasmedgeLoaderTests
  DescriptionTest.cpp
  ExpressionTest.cpp
  FileMgrTest.cpp
  InstructionTest.cpp
  ModuleTest.cpp
  SectionTest.cpp
  SegmentTest.cpp
  SerializeDescriptionTest.cpp
  SerializeExpressionTest.cpp
  SerializeInstructionTest.cpp
  SerializeModuleTest.cpp
  SerializeSectionTest.cpp
  SerializeSegmentTest.cpp
  SerializeTypeTest.cpp
  TypeTest.cpp
)

add_test(wasmedgeLoaderTests wasmedgeLoaderTests)

target_link_libraries(wasmedgeLoaderTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeLoader
)
