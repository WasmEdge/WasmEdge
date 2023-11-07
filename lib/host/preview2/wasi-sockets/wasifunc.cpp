// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/preview2/wasi-sockets/udp/wasifunc.h"
#include "common/log.h"
#include "executor/executor.h"
#include "host/preview2/wasi-sockets/api.h"
#include "host/wasi/environ.h"
#include "runtime/instance/memory.h"

#include <algorithm>
#include <array>
#include <limits>
#include <numeric>
#include <type_traits>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiSocket {
namespace {} // namespace

} // namespace WasiSocket
} // namespace Host
} // namespace WasmEdge
