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

target_include_directories(ssvmHostModuleEEI
  PUBLIC
  ${Boost_INCLUDE_DIR}
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
  ${PROJECT_SOURCE_DIR}/thirdparty/evmc/include
)
