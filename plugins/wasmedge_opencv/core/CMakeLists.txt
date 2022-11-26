# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

add_library(core 
	SHARED
	coremodule.cpp
	corefunc.cpp
)

target_include_directories(core
  PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/.."

  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(core
  PUBLIC
  ${EXTRA_LIBS}
  ${OpenCV_LIBS}
)


