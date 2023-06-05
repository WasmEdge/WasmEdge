# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

# Don't reply on System zlib
# find_package(ZLIB REQUIRED)

set(ZLIB_COMPAT ON)
set(ZLIBNG_ENABLE_TESTS OFF)

FetchContent_Declare(
    ZLIB
    GIT_REPOSITORY "https://github.com/zlib-ng/zlib-ng.git"
    GIT_TAG 2.0.7
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(ZLIB)

