# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2026 Second State INC

wasmedge_add_executable(wasmedgeComponentFuzzTest
  fuzz_component.cpp
)

target_link_libraries(wasmedgeComponentFuzzTest
  PRIVATE
  wasmedgeLoader
  wasmedgeValidator
)

target_include_directories(wasmedgeComponentFuzzTest
  PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)
