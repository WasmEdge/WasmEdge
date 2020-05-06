# SPDX-License-Identifier: Apache-2.0

add_library(ssvmInterpreterEngine
  control.cpp
  memory.cpp
  variable.cpp
  provider.cpp
  engine.cpp
)

target_link_libraries(ssvmInterpreterEngine
  PRIVATE
  ssvmSupport
)

target_include_directories(ssvmInterpreterEngine
  PUBLIC
  ${PROJECT_SOURCE_DIR}/include
)
