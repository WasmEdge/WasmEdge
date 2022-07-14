# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

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
