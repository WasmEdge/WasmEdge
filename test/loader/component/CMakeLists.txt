file(COPY
  ${CMAKE_CURRENT_SOURCE_DIR}/testData
  DESTINATION
  ${CMAKE_CURRENT_BINARY_DIR}
  )

wasmedge_add_executable(wasmedgeLoaderComponentASTTests
  type_definition.cpp
  )

add_test(wasmedgeLoaderComponentASTTests wasmedgeLoaderComponentASTTests)

target_link_libraries(wasmedgeLoaderComponentASTTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeLoader
  )
