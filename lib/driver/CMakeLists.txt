# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

wasmedge_add_library(wasmedgeDriver
  compilerTool.cpp
  runtimeTool.cpp
  fuzzTool.cpp
  fuzzPO.cpp
)

if(WASMEDGE_BUILD_AOT_RUNTIME)
  target_link_libraries(wasmedgeDriver
    PRIVATE
    wasmedgeLoader
    wasmedgeCommon
    wasmedgePO
    wasmedgeVM
    wasmedgeAOT
  )
else()
  target_link_libraries(wasmedgeDriver
    PRIVATE
    wasmedgeLoader
    wasmedgeCommon
    wasmedgePO
    wasmedgeVM
  )
endif()

if(WASMEDGE_BUILD_AOT_RUNTIME)
  target_compile_definitions(wasmedgeDriver
    PRIVATE
    -DWASMEDGE_BUILD_AOT_RUNTIME
  )
endif()
