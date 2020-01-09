# SPDX-License-Identifier: Apache-2.0

add_library(ssvmHostFuncEEI
  callDataCopy.cpp
  callStatic.cpp
  finish.cpp
  getCallDataSize.cpp
  getCaller.cpp
  returnDataCopy.cpp
  revert.cpp
  storageLoad.cpp
  storageStore.cpp
)

target_link_libraries(ssvmHostFuncEEI
  PRIVATE
  utilKeccak
)
