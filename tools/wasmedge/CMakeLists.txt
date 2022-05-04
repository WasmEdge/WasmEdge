# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

file(COPY
  ${CMAKE_CURRENT_SOURCE_DIR}/examples
  DESTINATION
  ${CMAKE_CURRENT_BINARY_DIR}
)

if(WASMEDGE_BUILD_AOT_RUNTIME)
  wasmedge_add_executable(wasmedgec
    wasmedgec.cpp
  )

  install(TARGETS wasmedgec EXPORT wasmedge DESTINATION bin)

  target_link_libraries(wasmedgec
    PRIVATE
    wasmedgeLoader
    wasmedgeCommon
    wasmedgePO
    wasmedgeValidator
    wasmedgeAOT
    std::filesystem
  )
endif()

wasmedge_add_executable(wasmedge
  wasmedger.cpp
)

install(TARGETS wasmedge EXPORT wasmedge DESTINATION bin)

target_link_libraries(wasmedge
  PRIVATE
  wasmedgePO
  wasmedgeVM
)
