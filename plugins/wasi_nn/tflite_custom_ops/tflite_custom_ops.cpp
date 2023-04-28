// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "tflite_custom_ops.h"
#include "shared_library.h"

namespace WasmEdge {
namespace Host {
namespace WASINNTfLite {

namespace {
using AddCustomOpsFuncType = decltype(wasmEdgeWasiNnTfLiteAddCustomOps);
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

static constexpr char const *EnvKeyName =
    "WASMEDGE_PLUGIN_WASI_NN_TFLITE_CUSTOM_OPS_PATH";

static void *findSymbolFromSharedLibrary() {
  static SharedLibrary CustomOpLib = SharedLibrary::loadFromEnv(EnvKeyName);

  if (!CustomOpLib.isValid()) {
    return nullptr;
  }

  const auto *SymbolName = TOSTRING(wasmEdgeWasiNnTfLiteAddCustomOps);
  auto *Symbol = CustomOpLib.getSymbolAddr(SymbolName);
  if (Symbol) {
    spdlog::debug("[WASI-NN] Find symbol \"{}\" from shared library success.",
                  SymbolName);
  } else {
    spdlog::error(
        "[WASI-NN] Invalid shared library, cannot find symbol \"{}\".",
        SymbolName);
  }

  return Symbol;
}

static void *AddCustomOpsFunc = findSymbolFromSharedLibrary();

} // namespace

void tfLiteInterpreterOptionsAddCustomOps(TfLiteInterpreterOptions *Options) {
  if (AddCustomOpsFunc) {
    reinterpret_cast<AddCustomOpsFuncType *>(AddCustomOpsFunc)(Options);
  }
}

} // namespace WASINNTfLite
} // namespace Host
} // namespace WasmEdge
