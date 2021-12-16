// SPDX-License-Identifier: Apache-2.0
#include "executor/engine/atomic_helper.h"

#include <mutex>

namespace WasmEdge {
namespace Executor {
namespace detail {

std::mutex wasmedge_atomic_lock;

void atomicLock() { wasmedge_atomic_lock.lock(); }
void atomicUnlock() { wasmedge_atomic_lock.unlock(); }

} // namespace detail
} // namespace Executor
} // namespace WasmEdge
