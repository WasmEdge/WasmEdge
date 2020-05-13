# SPDX-License-Identifier: Apache-2.0
add_executable(ssvmc
  ssvmc.cpp
)

target_link_libraries(ssvmc
  PRIVATE
  ssvmLoader
  ssvmAOT
  std::filesystem
)

add_executable(ssvmr
  ssvmr.cpp
)

target_link_libraries(ssvmr
  PRIVATE
  ssvmExpVM
  std::filesystem
)
