# SPDX-License-Identifier: Apache-2.0

add_library(ssvmInterpreterInstantiate
  import.cpp
  function.cpp
  global.cpp
  table.cpp
  memory.cpp
  elem.cpp
  data.cpp
  export.cpp
  module.cpp
)

target_link_libraries(ssvmInterpreterInstantiate
  PRIVATE
  ssvmSupport
)

target_include_directories(ssvmInterpreterInstantiate
  PUBLIC
  ${Boost_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
)
