# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_executable(componentTests
  spectest.cpp
  componet_nameparser.cpp
  componentvalidatortest.cpp
)

add_test(componentTests componentTests)

target_link_libraries(componentTests
  PRIVATE
  wasmedgeVM
  ${GTEST_BOTH_LIBRARIES}
)

target_include_directories(componentTests
  PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)

add_subdirectory(fuzzing)
