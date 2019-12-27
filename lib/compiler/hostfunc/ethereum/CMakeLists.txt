# SPDX-License-Identifier: Apache-2.0

add_library(ssvmCompilerHostFuncEEI
  getcalldatasize.cpp
  calldatacopy.cpp
  storagestore.cpp
  storageload.cpp
  finish.cpp
  revert.cpp
  callstatic.cpp
  getcaller.cpp
  returndatacopy.cpp
)

target_link_libraries(ssvmCompilerHostFuncEEI
  PRIVATE
  utilKeccak
)
