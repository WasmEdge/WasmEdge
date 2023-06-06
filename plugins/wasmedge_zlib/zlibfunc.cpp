// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "zlibfunc.h"

#include <cstring>
#include <iostream>

namespace WasmEdge {
namespace Host {

Expect<int32_t>
WasmEdgeZlibDeflateInit_::body(const Runtime::CallingFrame &Frame,
                               uint32_t ZStreamPtr, int32_t Level,
                               uint32_t VersionPtr, uint32_t StreamSize) {

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  // Wasm_z_stream *WasmZStream = MemInst->getPointer<Wasm_z_stream
  // *>(ZStreamPtr);
  if (StreamSize != 56) {
    spdlog::error("[WasmEdge Zlib] [WasmEdgeZlibDeflateInit_] WASM "
                  "sizeof(z_stream) != 56 but {}",
                  StreamSize);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const char *WasmZlibVersion = MemInst->getPointer<const char *>(VersionPtr);
  if (WasmZlibVersion[0] != ZLIB_VERSION[0]) {
    spdlog::error("[WasmEdge Zlib] [WasmEdgeZlibDeflateInit_] Major Zlib "
                  "version of Host ({}) & Wasm ({}) "
                  "does not match",
                  ZLIB_VERSION[0], WasmZlibVersion[0]);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto HostZStream = std::make_unique<z_stream>();

  HostZStream.get()->zalloc = Z_NULL;
  HostZStream.get()->zfree = Z_NULL;
  HostZStream.get()->opaque =
      Z_NULL; // ignore opaque since zmalloc and zfree was ignored

  const auto z_res =
      deflateInit_(HostZStream.get(), Level, ZLIB_VERSION, sizeof(z_stream));

  Env.ZStreamMap.emplace(std::make_pair(ZStreamPtr, std::move(HostZStream)));

  return static_cast<int32_t>(z_res);
}

} // namespace Host
} // namespace WasmEdge
