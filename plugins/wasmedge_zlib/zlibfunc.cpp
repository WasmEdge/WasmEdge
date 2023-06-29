// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "zlibfunc.h"

#include <cstring>
#include <iostream>

constexpr bool CheckVersionSize(const char *WasmZlibVersion,
                                int32_t StreamSize) {
  /*
  Reason behind just comparing the first character of zlib version strings.
  https://github.com/zlib-ng/zlib-ng/blob/2f4ebe2bb68380366b90f1db1f3c5b32601130a0/zutil.h#L130
  https://github.com/madler/zlib/blob/04f42ceca40f73e2978b50e93806c2a18c1281fc/inflate.c#L207
  */

  return (WasmZlibVersion != 0 && WasmZlibVersion[0] == ZLIB_VERSION[0] &&
          StreamSize == (int32_t)sizeof(WasmZStream));
}

namespace WasmEdge {
namespace Host {

Expect<int32_t>
WasmEdgeZlibDeflateInit_::body(const Runtime::CallingFrame &Frame,
                               uint32_t ZStreamPtr, int32_t Level,
                               uint32_t VersionPtr, int32_t StreamSize) {

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const char *WasmZlibVersion = MemInst->getPointer<const char *>(VersionPtr);
  if (!CheckVersionSize(WasmZlibVersion, StreamSize))
    return static_cast<int32_t>(Z_VERSION_ERROR);

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
                               int32_t StreamSize) {

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const char *WasmZlibVersion = MemInst->getPointer<const char *>(VersionPtr);
  if (!CheckVersionSize(WasmZlibVersion, StreamSize))
    return static_cast<int32_t>(Z_VERSION_ERROR);

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
  WasmZStream *ModuleZStream = MemInst->getPointer<WasmZStream *>(ZStreamPtr);

  HostZStream->avail_in = ModuleZStream->avail_in;   // value
  HostZStream->avail_out = ModuleZStream->avail_out; // value
  HostZStream->next_in =
      MemInst->getPointer<unsigned char *>(ModuleZStream->next_in); // ptr
  HostZStream->next_out =
      MemInst->getPointer<unsigned char *>(ModuleZStream->next_out); // ptr

  const auto PreComputeNextIn = HostZStream->next_in;
  const auto PreComputeNextOut = HostZStream->next_out;

  const auto z_res = deflate(HostZStream, Flush);

  ModuleZStream->avail_in = HostZStream->avail_in;
  ModuleZStream->avail_out = HostZStream->avail_out;
  ModuleZStream->next_in += HostZStream->next_in - PreComputeNextIn;
  ModuleZStream->next_out += HostZStream->next_out - PreComputeNextOut;

  return static_cast<int32_t>(z_res);
}

Expect<int32_t> WasmEdgeZlibInflate::body(const Runtime::CallingFrame &Frame,
                                          uint32_t ZStreamPtr, int32_t Flush) {

  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  auto HostZStream = HostZStreamIt->second.get();
  auto *MemInst = Frame.getMemoryByIndex(0);
  WasmZStream *ModuleZStream = MemInst->getPointer<WasmZStream *>(ZStreamPtr);

  HostZStream->avail_in = ModuleZStream->avail_in;   // value
  HostZStream->avail_out = ModuleZStream->avail_out; // value
  HostZStream->next_in =
      MemInst->getPointer<unsigned char *>(ModuleZStream->next_in); // ptr
  HostZStream->next_out =
      MemInst->getPointer<unsigned char *>(ModuleZStream->next_out); // ptr

  const auto PreComputeNextIn = HostZStream->next_in;
  const auto PreComputeNextOut = HostZStream->next_out;

  const auto z_res = inflate(HostZStream, Flush);

  ModuleZStream->avail_in = HostZStream->avail_in;
  ModuleZStream->avail_out = HostZStream->avail_out;
  ModuleZStream->next_in += HostZStream->next_in - PreComputeNextIn;
  ModuleZStream->next_out += HostZStream->next_out - PreComputeNextOut;

  return static_cast<int32_t>(z_res);
}

Expect<int32_t> WasmEdgeZlibDeflateEnd::body(const Runtime::CallingFrame &,
                                             uint32_t ZStreamPtr) {
  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  auto HostZStream = HostZStreamIt->second.get();
  int32_t ZRes = deflateEnd(HostZStream);

  Env.ZStreamMap.erase(ZStreamPtr);

  return static_cast<int32_t>(ZRes);
}

Expect<int32_t> WasmEdgeZlibInflateEnd::body(const Runtime::CallingFrame &,
                                             uint32_t ZStreamPtr) {
  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  auto HostZStream = HostZStreamIt->second.get();
  int32_t ZRes = inflateEnd(HostZStream);

  Env.ZStreamMap.erase(ZStreamPtr);

  return static_cast<int32_t>(ZRes);
}

} // namespace Host
} // namespace WasmEdge

/*
TODO:
sync *msg in [inflate|deflate]End()
*/
