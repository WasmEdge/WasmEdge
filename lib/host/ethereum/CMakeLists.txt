# SPDX-License-Identifier: Apache-2.0

add_library(ssvmHostModuleEEI
  eeienv.cpp
  eeifunc.cpp
  eeimodule.cpp
)

target_link_libraries(ssvmHostModuleEEI
  PRIVATE
  utilKeccak
)
