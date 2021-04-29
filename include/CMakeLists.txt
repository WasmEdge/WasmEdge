# SPDX-License-Identifier: Apache-2.0

# Add the git command here to support describing version without the root CMakeList.
find_program(GIT_CMD git)
execute_process(COMMAND
  ${GIT_CMD} describe --match "*[0-9].[0-9]*" --tag
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_VARIABLE SSVM_GIT_VERSION
  RESULT_VARIABLE GIT_VERSION_NOT_FOUND
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(GIT_VERSION_NOT_FOUND AND NOT GIT_VERSION_NOT_FOUND EQUAL 0)
  set(SSVM_GIT_VERSION "0.0.0-unreleased")
endif()

string(REGEX MATCH "^([0-9]+)[.]([0-9]+)[.]([0-9]+)*"
    SSVM_VERSION_STRING
    "${SSVM_GIT_VERSION}"
)
string(REPLACE "." ";" SSVM_VERSION_LIST ${SSVM_VERSION_STRING})
list(GET SSVM_VERSION_LIST 0 SSVM_VERSION_MAJOR)
list(GET SSVM_VERSION_LIST 1 SSVM_VERSION_MINOR)
list(GET SSVM_VERSION_LIST 2 SSVM_VERSION_PATCH)
configure_file(common/config.h.in common/config.h)
configure_file(common/version.h.in common/version.h)
configure_file(api/ssvm.h.in api/ssvm.h)
unset(SSVM_VERSION_STRING)
unset(SSVM_VERSION_LIST)
unset(SSVM_VERSION_MAJOR)
unset(SSVM_VERSION_MINOR)
unset(SSVM_VERSION_PATCH)