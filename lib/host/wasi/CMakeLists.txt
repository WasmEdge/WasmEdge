# SPDX-License-Identifier: Apache-2.0

add_library(ssvmHostModuleWasi
  wasienv.cpp
  wasifunc.cpp
  wasimodule.cpp
)

target_include_directories(ssvmHostModuleWasi
  PUBLIC
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
)

target_link_libraries(ssvmHostModuleWasi
  PUBLIC
  rt
)
