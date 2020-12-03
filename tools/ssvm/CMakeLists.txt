# SPDX-License-Identifier: Apache-2.0
configure_files(
  ${CMAKE_CURRENT_SOURCE_DIR}/examples
  ${CMAKE_CURRENT_BINARY_DIR}/examples
  COPYONLY
)

if (NOT SSVM_DISABLE_AOT_RUNTIME)
  add_executable(ssvmc
    ssvmc.cpp
  )

  install(TARGETS ssvmc EXPORT ssvm DESTINATION bin)

  target_link_libraries(ssvmc
    PRIVATE
    ssvmLoader
    ssvmValidator
    ssvmAOT
    std::filesystem
  )
endif()

add_executable(ssvm
  ssvmr.cpp
)

install(TARGETS ssvm EXPORT ssvm DESTINATION bin)

target_link_libraries(ssvm
  PRIVATE
  ssvmVM
)
