// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "runtime/instance/memory.h"

namespace WasmEdge {
namespace Host {

/// \brief Read a C string from memory and check whether it is null terminated.
/// \param memory memory instance from the wasm runtime.
/// \param ptr the wasm32 buffer pointer
/// \return
WasmEdge::Expect<const char *>
read_c_str(WasmEdge::Runtime::Instance::MemoryInstance *memory, uint32_t ptr);

/// Check exist and set cstr, or return `Unexpect`.
#define checkAndSetCstr(memory, name, name_str)                                \
  do {                                                                         \
    if (auto res = read_c_str(memory, name); unlikely(!res)) {                 \
      return Unexpect(res.error());                                            \
    } else {                                                                   \
      name_str = *res;                                                         \
    }                                                                          \
  } while (0)

} // namespace Host
} // namespace WasmEdge
