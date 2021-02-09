# SPDX-License-Identifier: Apache-2.0
string(REGEX MATCH "^([0-9]+)[.]([0-9]+)[.]([0-9]+)*"
    SSVM_VERSION_STRING
    "${CPACK_PACKAGE_VERSION}"
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