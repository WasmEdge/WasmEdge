// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace WasmEdge {
namespace Executor {
namespace detail {

void atomicLock();
void atomicUnlock();

} // namespace detail
} // namespace Executor
} // namespace WasmEdge
