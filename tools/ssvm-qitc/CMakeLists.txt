# SPDX-License-Identifier: Apache-2.0
add_executable(ssvm-qitc
  main.cpp
)

target_link_libraries(ssvm-qitc
  PRIVATE
  ssvmExpVM
)
