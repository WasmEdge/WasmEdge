// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include <tensorflow/lite/c/c_api.h>

/// function(symbol) declare
/// used for shared custom ops library
extern "C" [[maybe_unused]] void
wasmEdgeWasiNnTfLiteAddCustomOps(TfLiteInterpreterOptions *);

namespace WasmEdge {
namespace Host {
namespace WASINNTfLite {

void tfLiteInterpreterOptionsAddCustomOps(TfLiteInterpreterOptions *);

} // namespace WASINNTfLite
} // namespace Host
} // namespace WasmEdge
