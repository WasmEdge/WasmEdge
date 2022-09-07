file(COPY
  ${CMAKE_CURRENT_SOURCE_DIR}/testData
  DESTINATION
  ${CMAKE_CURRENT_BINARY_DIR}
)

wasmedge_add_executable(wasmedgeLoaderComponentASTTests
  alias_definition.cpp
  core_module.cpp
  type_definition.cpp
  core_type_definition.cpp
  import_export_definition.cpp
  instance_definition.cpp
  nested_component.cpp
  type_definition.cpp
)

add_test(wasmedgeLoaderComponentASTTests wasmedgeLoaderComponentASTTests)

target_link_libraries(wasmedgeLoaderComponentASTTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeLoader
)
