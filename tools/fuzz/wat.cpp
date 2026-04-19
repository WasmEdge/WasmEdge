// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

#include "wasmedge/wasmedge.h"
#include <stddef.h>

/// Entrypoint for the WAT fuzz tool.
WASMEDGE_CAPI_EXPORT extern "C" int WasmEdge_Driver_FuzzWAT(const uint8_t *Data,
                                                            size_t Size) noexcept;

extern "C" [[gnu::visibility("default")]] int
LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
  return WasmEdge_Driver_FuzzWAT(Data, Size);
}
