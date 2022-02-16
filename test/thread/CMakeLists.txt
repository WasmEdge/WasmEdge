# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

wasmedge_add_executable(wasmedgeThreadTests
  ThreadTest.cpp
)

add_test(wasmedgeThreadTests wasmedgeThreadTests)

target_link_libraries(wasmedgeThreadTests
  PRIVATE
  std::filesystem
  ${GTEST_BOTH_LIBRARIES}
  wasmedgeVM
)

if(WASMEDGE_BUILD_AOT_RUNTIME)
  target_compile_definitions(wasmedgeThreadTests
    PRIVATE
    -DWASMEDGE_BUILD_AOT_RUNTIME
  )
  target_link_libraries(wasmedgeThreadTests
    PRIVATE
    wasmedgeAOT
  )
endif()
