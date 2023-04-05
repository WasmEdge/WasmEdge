// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "zlibmodule.h"
#include "zlibfunc.h"

namespace WasmEdge {
namespace Host {

/// Register your functions in module.
WasmEdgeZlibModule::WasmEdgeZlibModule() : ModuleInstance("wasmedge_zlib") {
  addHostFunc("wasmedge_zlib_deflateInit_",
              std::make_unique<WasmEdgeZlibDeflateInit_>(Env));
  addHostFunc("wasmedge_zlib_inflateInit_",
              std::make_unique<WasmEdgeZlibInflateInit_>(Env));
  addHostFunc("wasmedge_zlib_deflate",
              std::make_unique<WasmEdgeZlibDeflate>(Env));
  addHostFunc("wasmedge_zlib_inflate",
              std::make_unique<WasmEdgeZlibDeflate>(Env));
  addHostFunc("wasmedge_zlib_deflateEnd",
              std::make_unique<WasmEdgeZlibDeflateEnd>(Env));
  addHostFunc("wasmedge_zlib_inflateEnd",
              std::make_unique<WasmEdgeZlibDeflateEnd>(Env));
}

} // namespace Host
} // namespace WasmEdge
