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
  set(WASMEDGE_GIT_VERSION "0.0.0-unreleased")
endif()

string(REGEX MATCH "^([0-9]+)[.]([0-9]+)[.]([0-9]+)*"
    WASMEDGE_VERSION_STRING
    "${WASMEDGE_GIT_VERSION}"
)
string(REPLACE "." ";" WASMEDGE_VERSION_LIST ${WASMEDGE_VERSION_STRING})
list(GET WASMEDGE_VERSION_LIST 0 WASMEDGE_VERSION_MAJOR)
list(GET WASMEDGE_VERSION_LIST 1 WASMEDGE_VERSION_MINOR)
list(GET WASMEDGE_VERSION_LIST 2 WASMEDGE_VERSION_PATCH)

# Check the MMAP and PWD exists.
include(CheckCXXSymbolExists)
check_cxx_symbol_exists(mmap sys/mman.h HAVE_MMAP)
include(CheckIncludeFileCXX)
CHECK_INCLUDE_FILE_CXX(pwd.h HAVE_PWD_H)

configure_file(api/wasmedge/int128.h api/wasmedge/int128.h COPYONLY)
configure_file(api/wasmedge/version.h.in api/wasmedge/version.h)
configure_file(api/wasmedge/wasmedge.h api/wasmedge/wasmedge.h COPYONLY)
configure_file(common/config.h.in common/config.h)
configure_file(common/dense_enum_map.h api/wasmedge/dense_enum_map.h COPYONLY)
configure_file(common/enum.inc api/wasmedge/enum.inc COPYONLY)
configure_file(common/enum_configure.h api/wasmedge/enum_configure.h COPYONLY)
configure_file(common/enum_errcode.h api/wasmedge/enum_errcode.h COPYONLY)
configure_file(common/enum_types.h api/wasmedge/enum_types.h COPYONLY)
configure_file(common/spare_enum_map.h api/wasmedge/spare_enum_map.h COPYONLY)
configure_file(common/version.h.in common/version.h)
unset(WASMEDGE_VERSION_STRING)
unset(WASMEDGE_VERSION_LIST)
unset(WASMEDGE_VERSION_MAJOR)
unset(WASMEDGE_VERSION_MINOR)
unset(WASMEDGE_VERSION_PATCH)
