// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#ifndef _WASM_BPF_UTIL
#define _WASM_BPF_UTIL
#include "runtime/instance/memory.h"

WasmEdge::Expect<const char *>
read_c_str(WasmEdge::Runtime::Instance::MemoryInstance *memory, uint32_t ptr);

#endif