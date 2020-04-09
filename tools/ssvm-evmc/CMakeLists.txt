# SPDX-License-Identifier: Apache-2.0

add_library(ssvmEVMC
  SHARED
  evmccreate.cpp
)

target_link_libraries(ssvmEVMC
  PRIVATE
  ssvmVM
  ssvmHostModuleEEI
)
