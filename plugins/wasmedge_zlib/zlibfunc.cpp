// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "zlibfunc.h"

#include <cstring>
#include <iostream>

struct Wasm_z_stream {
  uint32_t next_in;
  uint32_t avail_in;
  uint32_t total_in;

  uint32_t next_out;
  uint32_t avail_out;
  uint32_t total_out;

  uint32_t msg;
  uint32_t state;

  uint32_t zalloc;
  uint32_t zfree;
  uint32_t opaque;

  int32_t data_type; // +ve & 2's complement in memory so int ~ uint

  uint32_t adler;
  uint32_t reserved;
}; // 56

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
  Wasm_z_stream *WasmZStream = MemInst->getPointer<Wasm_z_stream *>(ZStreamPtr);
  if (StreamSize != 56) {
    spdlog::error("[WasmEdge Zlib] WASM sizeof(z_stream) != 56 but {}",
                  StreamSize);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const char *WasmZlibVersion = MemInst->getPointer<const char *>(VersionPtr);
  if (WasmZlibVersion[0] != ZLIB_VERSION[0]) {
    spdlog::error("[WasmEdge Zlib] Major Zlib version of Host ({}) & Wasm ({}) "
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

Expect<int32_t>
WasmEdgeZlibInflateInit_::body(const Runtime::CallingFrame &Frame,
                               uint32_t ZStreamPtr, uint32_t VersionPtr,
                               uint32_t StreamSize) {

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  Wasm_z_stream *WasmZStream = MemInst->getPointer<Wasm_z_stream *>(ZStreamPtr);
  if (StreamSize != 56) {
    spdlog::error("[WasmEdge Zlib] WASM sizeof(z_stream) != 56 but {}",
                  StreamSize);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const char *WasmZlibVersion = MemInst->getPointer<const char *>(VersionPtr);
  if (WasmZlibVersion[0] != ZLIB_VERSION[0]) {
    spdlog::error("[WasmEdge Zlib] Major Zlib version of Host ({}) & Wasm ({}) "
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
      inflateInit_(HostZStream.get(), ZLIB_VERSION, sizeof(z_stream));

  Env.ZStreamMap.emplace(std::make_pair(ZStreamPtr, std::move(HostZStream)));

  return static_cast<int32_t>(z_res);
}

Expect<int32_t> WasmEdgeZlibDeflate::WasmEdgeZlibDeflate::body(
    const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr, int32_t Flush) {
  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  auto HostZStream = HostZStreamIt->second.get();

  auto *MemInst = Frame.getMemoryByIndex(0);
  return -1;
}

Expect<int32_t> WasmEdgeZlibInflate::body(const Runtime::CallingFrame &Frame,
                                          uint32_t ZStreamPtr, int32_t Flush) {
  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  auto HostZStream = HostZStreamIt->second.get();

  auto *MemInst = Frame.getMemoryByIndex(0);

  return -1;
}

Expect<int32_t> WasmEdgeZlibDeflateEnd::body(const Runtime::CallingFrame &Frame,
                                             uint32_t ZStreamPtr) {
  const auto HostZStreamIt = Env.ZStreamMap.get(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  auto HostZStream = *HostZStreamIt;
  int32_t ZRes = deflateEnd(HostZStream);

  // We dont need to sync the wasm ZStream
  // It *might set msg, so will later sync that

  Env.ZStreamMap.erase(ZStreamPtr);

  return static_cast<int32_t>(ZRes); // not really needed
}

Expect<int32_t> WasmEdgeZlibInflateEnd::body(const Runtime::CallingFrame &Frame,
                                             uint32_t ZStreamPtr) {
  const auto HostZStreamIt = Env.ZStreamMap.get(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  auto HostZStream = *HostZStreamIt;
  int32_t ZRes = inflateEnd(HostZStream);

  // We dont need to sync the wasm ZStream
  // It *might set msg, so wil later sync that

  Env.ZStreamMap.erase(ZStreamPtr);

  return static_cast<int32_t>(ZRes); // not really needed
}

} // namespace Host
} // namespace WasmEdge
