# SPDX-License-Identifier: Apache-2.0

add_subdirectory(ast)
add_subdirectory(core)
add_subdirectory(loader)
add_subdirectory(expected)
add_subdirectory(span)

if(BUILD_COVERAGE)
  setup_target_for_coverage_gcovr_html(
    NAME coverage
    EXECUTABLE ctest -j ${PROCESSOR_COUNT}
    DEPENDENCIES ssvm
    BASE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    EXCLUDE
      "${PROJECT_SOURCE_DIR}/thirdparty/*"
    )
  setup_target_for_coverage_gcovr_xml(
    NAME codecov
    EXECUTABLE ctest -j ${PROCESSOR_COUNT}
    DEPENDENCIES ssvm
    BASE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    EXCLUDE
      "${PROJECT_SOURCE_DIR}/thirdparty/*"
    )

endif()
