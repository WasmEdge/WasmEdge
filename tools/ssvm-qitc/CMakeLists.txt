# SPDX-License-Identifier: Apache-2.0
add_executable(ssvm-qitc
  main.cpp
)

install(TARGETS ssvm-qitc EXPORT ssvm DESTINATION bin)

target_link_libraries(ssvm-qitc
  PRIVATE
  ssvmVM
  ssvmHostModuleWasi
)
