// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "zlibfunc.h"

#include <cstring>
#include <iostream>

namespace WasmEdge {
namespace Host {

constexpr bool CheckSize(int32_t StreamSize) {

  return (StreamSize == static_cast<int32_t>(sizeof(WasmZStream)));
}

template <typename T>
auto SyncRun(z_stream *HostZStream, uint32_t ZStreamPtr,
             const Runtime::CallingFrame &Frame, T Callback) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  WasmZStream *ModuleZStream = MemInst->getPointer<WasmZStream *>(ZStreamPtr);

  HostZStream->next_in =
      MemInst->getPointer<unsigned char *>(ModuleZStream->next_in);
  HostZStream->avail_in = ModuleZStream->avail_in;
  HostZStream->total_in = ModuleZStream->total_in;

  HostZStream->next_out =
      MemInst->getPointer<unsigned char *>(ModuleZStream->next_out);
  HostZStream->avail_out = ModuleZStream->avail_out;
  HostZStream->total_out = ModuleZStream->total_out;

  // TODO: ignore msg for now
  // ignore state
  // ignore zalloc, zfree, opaque

  HostZStream->data_type = ModuleZStream->data_type;
  HostZStream->adler = ModuleZStream->adler;
  HostZStream->reserved = ModuleZStream->reserved;

  const auto PreComputeNextIn = HostZStream->next_in;
  const auto PreComputeNextOut = HostZStream->next_out;

  const auto ZRes = Callback();

  ModuleZStream->next_in += HostZStream->next_in - PreComputeNextIn;
  ModuleZStream->avail_in = HostZStream->avail_in;
  ModuleZStream->total_in = HostZStream->total_in;

  ModuleZStream->next_out += HostZStream->next_out - PreComputeNextOut;
  ModuleZStream->avail_out = HostZStream->avail_out;
  ModuleZStream->total_out = HostZStream->total_out;

  // TODO: ignore msg for now
  // ignore state
  // ignore zalloc, zfree, opaque

  ModuleZStream->data_type = HostZStream->data_type;
  ModuleZStream->adler = HostZStream->adler;
  ModuleZStream->reserved = HostZStream->reserved;

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibDeflateInit_::body(const Runtime::CallingFrame &Frame,
                               uint32_t ZStreamPtr, int32_t Level,
                               uint32_t VersionPtr, int32_t StreamSize) {

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const auto *WasmZlibVersion = MemInst->getPointer<const char *>(VersionPtr);
  if (!CheckSize(StreamSize))
    return static_cast<int32_t>(Z_VERSION_ERROR);

  auto HostZStream = std::make_unique<z_stream>();

  HostZStream.get()->zalloc = Z_NULL;
  HostZStream.get()->zfree = Z_NULL;
  HostZStream.get()->opaque =
      Z_NULL; // ignore opaque since zmalloc and zfree was ignored

  const int32_t ZRes = SyncRun(HostZStream.get(), ZStreamPtr, Frame, [&]() {
    return deflateInit_(HostZStream.get(), Level, WasmZlibVersion,
                        sizeof(z_stream));
  });

  if (ZRes == Z_OK)
    Env.ZStreamMap.emplace(std::make_pair(ZStreamPtr, std::move(HostZStream)));

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibInflateInit_::body(const Runtime::CallingFrame &Frame,
                               uint32_t ZStreamPtr, uint32_t VersionPtr,
                               int32_t StreamSize) {

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const auto *WasmZlibVersion = MemInst->getPointer<const char *>(VersionPtr);
  if (!CheckSize(StreamSize))
    return static_cast<int32_t>(Z_VERSION_ERROR);

  auto HostZStream = std::make_unique<z_stream>();

  HostZStream.get()->zalloc = Z_NULL;
  HostZStream.get()->zfree = Z_NULL;
  HostZStream.get()->opaque =
      Z_NULL; // ignore opaque since zmalloc and zfree was ignored

  const int32_t ZRes = SyncRun(HostZStream.get(), ZStreamPtr, Frame, [&]() {
    return inflateInit_(HostZStream.get(), WasmZlibVersion, sizeof(z_stream));
  });

  if (ZRes == Z_OK)
    Env.ZStreamMap.emplace(std::make_pair(ZStreamPtr, std::move(HostZStream)));

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibDeflate::WasmEdgeZlibDeflate::body(
    const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr, int32_t Flush) {

  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const int32_t ZRes =
      SyncRun(HostZStreamIt->second.get(), ZStreamPtr, Frame,
              [&]() { return deflate(HostZStreamIt->second.get(), Flush); });

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibInflate::body(const Runtime::CallingFrame &Frame,
                                          uint32_t ZStreamPtr, int32_t Flush) {

  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const int32_t ZRes =
      SyncRun(HostZStreamIt->second.get(), ZStreamPtr, Frame,
              [&]() { return inflate(HostZStreamIt->second.get(), Flush); });

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibDeflateEnd::body(const Runtime::CallingFrame &Frame,
                                             uint32_t ZStreamPtr) {
  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const int32_t ZRes =
      SyncRun(HostZStreamIt->second.get(), ZStreamPtr, Frame,
              [&]() { return deflateEnd(HostZStreamIt->second.get()); });

  Env.ZStreamMap.erase(ZStreamPtr);

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibInflateEnd::body(const Runtime::CallingFrame &Frame,
                                             uint32_t ZStreamPtr) {
  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const int32_t ZRes =
      SyncRun(HostZStreamIt->second.get(), ZStreamPtr, Frame,
              [&]() { return inflateEnd(HostZStreamIt->second.get()); });

  Env.ZStreamMap.erase(ZStreamPtr);

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibDeflateSetDictionary::body(
    const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
    uint32_t DictionaryPtr, uint32_t DictLength) {

  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *MemInst = Frame.getMemoryByIndex(0);
  const auto *Dictionary = MemInst->getPointer<const Bytef *>(DictionaryPtr);

  const int32_t ZRes =
      SyncRun(HostZStreamIt->second.get(), ZStreamPtr, Frame, [&]() {
        return deflateSetDictionary(HostZStreamIt->second.get(), Dictionary,
                                    DictLength);
      });

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibDeflateGetDictionary::body(
    const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
    uint32_t DictionaryPtr, uint32_t DictLengthPtr) {

  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *MemInst = Frame.getMemoryByIndex(0);
  auto *Dictionary = MemInst->getPointer<Bytef *>(DictionaryPtr);
  auto *DictLength = MemInst->getPointer<uint32_t *>(DictLengthPtr);

  const int32_t ZRes =
      SyncRun(HostZStreamIt->second.get(), ZStreamPtr, Frame, [&]() {
        return deflateGetDictionary(HostZStreamIt->second.get(), Dictionary,
                                    DictLength);
      });

  return ZRes;
}

/*
"The deflateCopy() function shall copy the compression state information in
source to the uninitialized z_stream structure referenced by dest."

https://refspecs.linuxbase.org/LSB_3.0.0/LSB-Core-generic/LSB-Core-generic/zlib-deflatecopy-1.html
*/
Expect<int32_t>
WasmEdgeZlibDeflateCopy::body(const Runtime::CallingFrame &Frame,
                              uint32_t DestPtr, uint32_t SourcePtr) {

  const auto SourceZStreamIt = Env.ZStreamMap.find(SourcePtr);
  if (SourceZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto DestZStream = std::make_unique<z_stream>();

  SyncRun(SourceZStreamIt->second.get(), SourcePtr, Frame, []() { return 0; });

  const int32_t ZRes = SyncRun(DestZStream.get(), DestPtr, Frame, [&]() {
    return deflateCopy(DestZStream.get(), SourceZStreamIt->second.get());
  });

  if (ZRes == Z_OK)
    Env.ZStreamMap.emplace(std::make_pair(DestPtr, std::move(DestZStream)));

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibDeflateReset::body(const Runtime::CallingFrame &Frame,
                               uint32_t ZStreamPtr) {

  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const int32_t ZRes =
      SyncRun(HostZStreamIt->second.get(), ZStreamPtr, Frame,
              [&]() { return deflateReset(HostZStreamIt->second.get()); });

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibDeflateParams::body(const Runtime::CallingFrame &Frame,
                                uint32_t ZStreamPtr, int32_t Level,
                                int32_t Strategy) {

  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const int32_t ZRes =
      SyncRun(HostZStreamIt->second.get(), ZStreamPtr, Frame, [&]() {
        return deflateParams(HostZStreamIt->second.get(), Level, Strategy);
      });

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibDeflateTune::body(
    const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr, int32_t GoodLength,
    int32_t MaxLazy, int32_t NiceLength, int32_t MaxChain) {

  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const int32_t ZRes =
      SyncRun(HostZStreamIt->second.get(), ZStreamPtr, Frame, [&]() {
        return deflateTune(HostZStreamIt->second.get(), GoodLength, MaxLazy,
                           NiceLength, MaxChain);
      });

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibDeflateBound::body(const Runtime::CallingFrame &Frame,
                               uint32_t ZStreamPtr, uint32_t SourceLen) {

  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const int32_t ZRes =
      SyncRun(HostZStreamIt->second.get(), ZStreamPtr, Frame, [&]() {
        return deflateBound(HostZStreamIt->second.get(), SourceLen);
      });

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibDeflatePending::body(const Runtime::CallingFrame &Frame,
                                 uint32_t ZStreamPtr, uint32_t PendingPtr,
                                 uint32_t BitsPtr) {

  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *MemInst = Frame.getMemoryByIndex(0);
  auto *Pending = MemInst->getPointer<uint32_t *>(PendingPtr);
  auto *Bits = MemInst->getPointer<int32_t *>(BitsPtr);

  const int32_t ZRes =
      SyncRun(HostZStreamIt->second.get(), ZStreamPtr, Frame, [&]() {
        return deflatePending(HostZStreamIt->second.get(), Pending, Bits);
      });

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibDeflatePrime::body(const Runtime::CallingFrame &Frame,
                               uint32_t ZStreamPtr, int32_t Bits,
                               int32_t Value) {

  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const int32_t ZRes =
      SyncRun(HostZStreamIt->second.get(), ZStreamPtr, Frame, [&]() {
        return deflatePrime(HostZStreamIt->second.get(), Bits, Value);
      });

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibInflateSetDictionary::body(
    const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
    uint32_t DictionaryPtr, uint32_t DictLength) {

  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *MemInst = Frame.getMemoryByIndex(0);
  auto *Dictionary = MemInst->getPointer<Bytef *>(DictionaryPtr);

  const int32_t ZRes =
      SyncRun(HostZStreamIt->second.get(), ZStreamPtr, Frame, [&]() {
        return inflateSetDictionary(HostZStreamIt->second.get(), Dictionary,
                                    DictLength);
      });

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibInflateSetDictionary::body(
    const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
    uint32_t DictionaryPtr, uint32_t DictLength) {

  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *MemInst = Frame.getMemoryByIndex(0);
  auto *Dictionary = MemInst->getPointer<Bytef *>(DictionaryPtr);

  const int32_t ZRes =
      SyncRun(HostZStreamIt->second.get(), ZStreamPtr, Frame, [&]() {
        return inflateSetDictionary(HostZStreamIt->second.get(), Dictionary,
                                    DictLength);
      });

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibInflateSetDictionary::body(
    const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
    uint32_t DictionaryPtr, uint32_t DictLengthPtr) {

  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *MemInst = Frame.getMemoryByIndex(0);
  auto *Dictionary = MemInst->getPointer<Bytef *>(DictionaryPtr);
  auto *DictLength = MemInst->getPointer<uint32_t *>(DictLengthPtr);

  const int32_t ZRes =
      SyncRun(HostZStreamIt->second.get(), ZStreamPtr, Frame, [&]() {
        return inflateGetDictionary(HostZStreamIt->second.get(), Dictionary,
                                    DictLength);
      });

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibInflateSync::body(const Runtime::CallingFrame &Frame,
                              uint32_t ZStreamPtr) {

  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const int32_t ZRes =
      SyncRun(HostZStreamIt->second.get(), ZStreamPtr, Frame,
              [&]() { return inflateSync(HostZStreamIt->second.get()); });

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibInflateCopy::body(const Runtime::CallingFrame &Frame,
                              uint32_t DestPtr, uint32_t SourcePtr) {

  const auto SourceZStreamIt = Env.ZStreamMap.find(SourcePtr);
  if (SourceZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto DestZStream = std::make_unique<z_stream>();

  SyncRun(SourceZStreamIt->second.get(), SourcePtr, Frame, []() { return 0; });

  const int32_t ZRes = SyncRun(DestZStream.get(), DestPtr, Frame, [&]() {
    return inflateCopy(DestZStream.get(), SourceZStreamIt->second.get());
  });

  if (ZRes == Z_OK)
    Env.ZStreamMap.emplace(std::make_pair(DestPtr, std::move(DestZStream)));

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibInflateReset::body(const Runtime::CallingFrame &Frame,
                               uint32_t ZStreamPtr) {

  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const int32_t ZRes =
      SyncRun(HostZStreamIt->second.get(), ZStreamPtr, Frame,
              [&]() { return inflateReset(HostZStreamIt->second.get()); });

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibInflateReset2::body(const Runtime::CallingFrame &Frame,
                                uint32_t ZStreamPtr, int32_t WindowBits) {

  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const int32_t ZRes =
      SyncRun(HostZStreamIt->second.get(), ZStreamPtr, Frame, [&]() {
        return inflateReset2(HostZStreamIt->second.get(), WindowBits);
      });

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibInflatePrime::body(const Runtime::CallingFrame &Frame,
                               uint32_t ZStreamPtr, int32_t Bits,
                               int32_t Value) {

  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const int32_t ZRes =
      SyncRun(HostZStreamIt->second.get(), ZStreamPtr, Frame, [&]() {
        return inflatePrime(HostZStreamIt->second.get(), Bits, Value);
      });

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibInflateMark::body(const Runtime::CallingFrame &Frame,
                              uint32_t ZStreamPtr) {

  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const int32_t ZRes =
      SyncRun(HostZStreamIt->second.get(), ZStreamPtr, Frame,
              [&]() { return inflateMark(HostZStreamIt->second.get()); });

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibInflateBackEnd::body(const Runtime::CallingFrame &Frame,
                                 uint32_t ZStreamPtr) {
  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const int32_t ZRes =
      SyncRun(HostZStreamIt->second.get(), ZStreamPtr, Frame,
              [&]() { return inflateBackEnd(HostZStreamIt->second.get()); });

  Env.ZStreamMap.erase(ZStreamPtr);

  return ZRes;
}

} // namespace Host
} // namespace WasmEdge
