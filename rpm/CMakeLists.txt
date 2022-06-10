# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

# Add the git command here to support describing version without the root CMakeList.
find_program(GIT_CMD git)
execute_process(COMMAND
  ${GIT_CMD} describe --match "[0-9].[0-9]*" --tag
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_VARIABLE WASMEDGE_GIT_VERSION
  RESULT_VARIABLE GIT_VERSION_NOT_FOUND
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(GIT_VERSION_NOT_FOUND AND NOT GIT_VERSION_NOT_FOUND EQUAL 0)
  if(DEFINED ENV{VERSION})
    set(WASMEDGE_GIT_VERSION $ENV{VERSION})
  else()
    set(WASMEDGE_GIT_VERSION "0.0.0-unreleased")
  endif()
endif()

string(REPLACE "-" "~" WASMEDGE_SRPM_VERSION ${WASMEDGE_GIT_VERSION})
configure_file(wasmedge.spec.in wasmedge.spec @ONLY)
