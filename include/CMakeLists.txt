# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

string(REGEX MATCH "^([0-9]+)[.]([0-9]+)[.]([0-9]+)*"
    WASMEDGE_VERSION_STRING
    "${CPACK_PACKAGE_VERSION}"
)
string(REPLACE "." ";" WASMEDGE_VERSION_LIST ${WASMEDGE_VERSION_STRING})
list(GET WASMEDGE_VERSION_LIST 0 WASMEDGE_VERSION_MAJOR)
list(GET WASMEDGE_VERSION_LIST 1 WASMEDGE_VERSION_MINOR)
list(GET WASMEDGE_VERSION_LIST 2 WASMEDGE_VERSION_PATCH)
set(WASMEDGE_API_VERSION "4")

# Check the MMAP and PWD exists.
include(CheckCXXSymbolExists)
check_cxx_symbol_exists(mmap sys/mman.h HAVE_MMAP)
include(CheckIncludeFileCXX)
CHECK_INCLUDE_FILE_CXX(pwd.h HAVE_PWD_H)

configure_file(api/wasmedge/int128.h api/wasmedge/int128.h COPYONLY)
configure_file(api/wasmedge/version.h.in api/wasmedge/version.h)
configure_file(api/wasmedge/wasmedge.h api/wasmedge/wasmedge.h COPYONLY)
configure_file(api/wasmedge/wasmedge_ast.h api/wasmedge/wasmedge_ast.h COPYONLY)
configure_file(api/wasmedge/wasmedge_basic.h api/wasmedge/wasmedge_basic.h COPYONLY)
configure_file(api/wasmedge/wasmedge_compiler.h api/wasmedge/wasmedge_compiler.h COPYONLY)
configure_file(api/wasmedge/wasmedge_configure.h api/wasmedge/wasmedge_configure.h COPYONLY)
configure_file(api/wasmedge/wasmedge_context.h api/wasmedge/wasmedge_context.h COPYONLY)
configure_file(api/wasmedge/wasmedge_deprecated.h api/wasmedge/wasmedge_deprecated.h COPYONLY)
configure_file(api/wasmedge/wasmedge_execution.h api/wasmedge/wasmedge_execution.h COPYONLY)
configure_file(api/wasmedge/wasmedge_experimental.h api/wasmedge/wasmedge_experimental.h COPYONLY)
configure_file(api/wasmedge/wasmedge_instance.h api/wasmedge/wasmedge_instance.h COPYONLY)
configure_file(api/wasmedge/wasmedge_plugin.h api/wasmedge/wasmedge_plugin.h COPYONLY)
configure_file(api/wasmedge/wasmedge_tools.h api/wasmedge/wasmedge_tools.h COPYONLY)
configure_file(api/wasmedge/wasmedge_value.h api/wasmedge/wasmedge_value.h COPYONLY)
configure_file(api/wasmedge/wasmedge_vm.h api/wasmedge/wasmedge_vm.h COPYONLY)
configure_file(common/config.h.in common/config.h)
configure_file(common/enum.inc api/wasmedge/enum.inc COPYONLY)
configure_file(common/enum_configure.h api/wasmedge/enum_configure.h COPYONLY)
configure_file(common/enum_errcode.h api/wasmedge/enum_errcode.h COPYONLY)
configure_file(common/enum_types.h api/wasmedge/enum_types.h COPYONLY)
configure_file(common/version.h.in common/version.h)
unset(WASMEDGE_VERSION_STRING)
unset(WASMEDGE_VERSION_LIST)
unset(WASMEDGE_VERSION_MAJOR)
unset(WASMEDGE_VERSION_MINOR)
unset(WASMEDGE_VERSION_PATCH)
unset(WASMEDGE_API_VERSION)
