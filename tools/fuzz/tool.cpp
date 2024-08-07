// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "wasmedge/wasmedge.h"
#include <stddef.h>

/// Entrypoint for the fuzz tool.
WASMEDGE_CAPI_EXPORT extern "C" int
WasmEdge_Driver_FuzzTool(const uint8_t *Data, size_t Size);

extern "C" [[gnu::visibility("default")]] int
LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
  return WasmEdge_Driver_FuzzTool(Data, Size);
}
