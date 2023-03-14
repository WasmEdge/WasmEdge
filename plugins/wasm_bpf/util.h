// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "runtime/instance/memory.h"

namespace WasmEdge {
namespace Host {

WasmEdge::Expect<const char *>
read_c_str(WasmEdge::Runtime::Instance::MemoryInstance *memory, uint32_t ptr);

} // namespace Host
} // namespace WasmEdge
