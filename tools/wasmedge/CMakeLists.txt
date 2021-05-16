# SPDX-License-Identifier: Apache-2.0
configure_files(
  ${CMAKE_CURRENT_SOURCE_DIR}/examples
  ${CMAKE_CURRENT_BINARY_DIR}/examples
  COPYONLY
)

if (BUILD_AOT_RUNTIME)
  add_executable(wasmedgec
    wasmedgec.cpp
  )

  install(TARGETS wasmedgec EXPORT wasmedge DESTINATION bin)

  target_link_libraries(wasmedgec
    PRIVATE
    wasmedgeLoader
    wasmedgeValidator
    wasmedgeAOT
    std::filesystem
  )
endif()

add_executable(wasmedge
  wasmedger.cpp
)

install(TARGETS wasmedge EXPORT wasmedge DESTINATION bin)

target_link_libraries(wasmedge
  PUBLIC
  wasmedgeVM
)

if(BUILD_TOOL_WASMEDGE_STATIC)
  add_executable(wasmedge-static
    wasmedger-static.cpp
    )

  install(TARGETS wasmedge-static COMPONENT static EXPORT wasmedge-static DESTINATION bin)

  target_link_libraries(wasmedge-static
    PRIVATE
    -static
    wasmedgeVM
    )
endif()
