# SPDX-License-Identifier: Apache-2.0
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

add_executable(ssvmr
  ssvmr.cpp
)

install(TARGETS ssvmr EXPORT ssvm DESTINATION bin)

target_link_libraries(ssvmr
  PRIVATE
  ssvmVM
  std::filesystem
)
