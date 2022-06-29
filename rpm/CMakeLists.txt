# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

set(WASMEDGE_GIT_VERSION ${CPACK_PACKAGE_VERSION})

string(REPLACE "-" "~" WASMEDGE_SRPM_VERSION ${WASMEDGE_GIT_VERSION})
configure_file(wasmedge.spec.in wasmedge.spec @ONLY)

unset(WASMEDGE_GIT_VERSION)
unset(WASMEDGE_SRPM_VERSION)
