# SPDX-License-Identifier: Apache-2.0

add_library(ssvmHostModuleSSVMProcess
  processfunc.cpp
  processmodule.cpp
)

target_include_directories(ssvmHostModuleSSVMProcess
  PUBLIC
  ${Boost_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
)
