# SPDX-License-Identifier: Apache-2.0
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
  wasmedgeVM
)

if(WASMEDGE_BUILD_TOOL_WASMEDGE_STATIC)
  wasmedge_add_executable(wasmedge-static
    wasmedger-static.cpp
  )

  install(TARGETS wasmedge-static COMPONENT static EXPORT wasmedge-static DESTINATION bin)

  target_link_libraries(wasmedge-static
    PRIVATE
    wasmedgeVM
    -static
  )
endif()
