# SPDX-License-Identifier: Apache-2.0
add_executable(ssvmc
  ssvmc.cpp
)

target_link_libraries(ssvmc
  PRIVATE
  ssvmLoader
  ssvmAOT
)

add_executable(ssvmr
  ssvmr.cpp
)

target_link_libraries(ssvmr
  PRIVATE
  ssvmExpVM
)
