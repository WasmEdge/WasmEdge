// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "zlibmodule.h"
#include "zlibfunc.h"

namespace WasmEdge {
namespace Host {

/// Register your functions in module.
WasmEdgeZlibModule::WasmEdgeZlibModule() : ModuleInstance("wasmedge_zlib") {
  addHostFunc("deflateInit_", std::make_unique<WasmEdgeZlibDeflateInit_>(Env));
  addHostFunc("inflateInit_", std::make_unique<WasmEdgeZlibInflateInit_>(Env));
  addHostFunc("deflate", std::make_unique<WasmEdgeZlibDeflate>(Env));
  addHostFunc("inflate", std::make_unique<WasmEdgeZlibInflate>(Env));
  addHostFunc("deflateEnd", std::make_unique<WasmEdgeZlibDeflateEnd>(Env));
  addHostFunc("inflateEnd", std::make_unique<WasmEdgeZlibInflateEnd>(Env));
}

} // namespace Host
} // namespace WasmEdge
