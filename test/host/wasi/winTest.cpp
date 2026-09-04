// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/defines.h"
#include <gtest/gtest.h>
#if WASMEDGE_OS_WINDOWS

#include "../../../lib/host/wasi/win.h"

using namespace WasmEdge::Host::WASI::detail;
using namespace WasmEdge::winapi;

// Verify that every documented Windows getaddrinfo() WSA error code maps to the
// expected WASI errno. WSANO_DATA was previously missing, so getaddrinfo()
// returning it hit `default: assumingUnreachable()` and crashed the runtime.
// Regression test for: https://github.com/WasmEdge/WasmEdge/issues/4375
TEST(WinTest, fromWSAError) {
  EXPECT_EQ(fromWSAError(static_cast<int>(WSATRY_AGAIN_)),
            __WASI_ERRNO_AIAGAIN);
  EXPECT_EQ(fromWSAError(static_cast<int>(WSAEINVAL_)),
            __WASI_ERRNO_AIBADFLAG);
  EXPECT_EQ(fromWSAError(static_cast<int>(WSANO_RECOVERY_)),
            __WASI_ERRNO_AIFAIL);
  EXPECT_EQ(fromWSAError(static_cast<int>(WSAEAFNOSUPPORT_)),
            __WASI_ERRNO_AIFAMILY);
  EXPECT_EQ(fromWSAError(static_cast<int>(ERROR_NOT_ENOUGH_MEMORY_)),
            __WASI_ERRNO_AIMEMORY);
  EXPECT_EQ(fromWSAError(static_cast<int>(WSANO_DATA_)),
            __WASI_ERRNO_AINODATA);
  EXPECT_EQ(fromWSAError(static_cast<int>(WSAHOST_NOT_FOUND_)),
            __WASI_ERRNO_AINONAME);
  EXPECT_EQ(fromWSAError(static_cast<int>(WSATYPE_NOT_FOUND_)),
            __WASI_ERRNO_AISERVICE);
  EXPECT_EQ(fromWSAError(static_cast<int>(WSAESOCKTNOSUPPORT_)),
            __WASI_ERRNO_AISOCKTYPE);
}

#endif
