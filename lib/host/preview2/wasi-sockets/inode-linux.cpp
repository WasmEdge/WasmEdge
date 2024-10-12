// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "common/defines.h"
#if WASMEDGE_OS_LINUX

#include "../../wasi/linux.h"
#include "common/errcode.h"
#include "common/variant.h"
#include "host/wasi/environ.h"
#include "host/wasi/inode.h"
#include "host/wasi/vfs.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <string>
#include <string_view>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASI {} // namespace WASI
} // namespace Host
} // namespace WasmEdge
#endif