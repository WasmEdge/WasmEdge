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

static constexpr uint32_t WasmGZFileStart = sizeof(gzFile);

template <typename T>
auto SyncRun(z_stream *HostZStream, uint32_t ZStreamPtr,
             const Runtime::CallingFrame &Frame, T Callback) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  /* if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  } */

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
WasmEdgeZlibDeflateInit::body(const Runtime::CallingFrame &Frame,
                              uint32_t ZStreamPtr, int32_t Level) {

  auto HostZStream = std::make_unique<z_stream>();
  HostZStream->zalloc = Z_NULL;
  HostZStream->zfree = Z_NULL;
  HostZStream->opaque =
      Z_NULL; // ignore opaque since zmalloc and zfree was ignored

  const int32_t ZRes = SyncRun(HostZStream.get(), ZStreamPtr, Frame, [&]() {
    return deflateInit(HostZStream.get(), Level);
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

Expect<int32_t> WasmEdgeZlibDeflateEnd::body(const Runtime::CallingFrame &Frame,
                                             uint32_t ZStreamPtr) {

  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const int32_t ZRes =
      SyncRun(HostZStreamIt->second.get(), ZStreamPtr, Frame,
              [&]() { return deflateEnd(HostZStreamIt->second.get()); });

  if (ZRes == Z_OK)
    Env.ZStreamMap.erase(ZStreamPtr);

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibInflateInit::body(const Runtime::CallingFrame &Frame,
                              uint32_t ZStreamPtr) {

  auto HostZStream = std::make_unique<z_stream>();
  HostZStream->zalloc = Z_NULL;
  HostZStream->zfree = Z_NULL;
  HostZStream->opaque =
      Z_NULL; // ignore opaque since zmalloc and zfree was ignored

  const int32_t ZRes = SyncRun(HostZStream.get(), ZStreamPtr, Frame, [&]() {
    return inflateInit(HostZStream.get());
  });

  if (ZRes == Z_OK)
    Env.ZStreamMap.emplace(std::make_pair(ZStreamPtr, std::move(HostZStream)));

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

Expect<int32_t> WasmEdgeZlibDeflateInit2::body(
    const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr, int32_t Level,
    int32_t Method, int32_t WindowBits, int32_t MemLevel, int32_t Strategy) {

  auto HostZStream = std::make_unique<z_stream>();
  HostZStream->zalloc = Z_NULL;
  HostZStream->zfree = Z_NULL;
  HostZStream->opaque =
      Z_NULL; // ignore opaque since zmalloc and zfree was ignored

  const int32_t ZRes = SyncRun(HostZStream.get(), ZStreamPtr, Frame, [&]() {
    return deflateInit2(HostZStream.get(), Level, Method, WindowBits, MemLevel,
                        Strategy);
  });

  if (ZRes == Z_OK)
    Env.ZStreamMap.emplace(std::make_pair(ZStreamPtr, std::move(HostZStream)));

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
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

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
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

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
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

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

Expect<int32_t>
WasmEdgeZlibInflateInit2::body(const Runtime::CallingFrame &Frame,
                               uint32_t ZStreamPtr, int32_t WindowBits) {

  auto HostZStream = std::make_unique<z_stream>();
  HostZStream->zalloc = Z_NULL;
  HostZStream->zfree = Z_NULL;
  HostZStream->opaque =
      Z_NULL; // ignore opaque since zmalloc and zfree was ignored

  const int32_t ZRes = SyncRun(HostZStream.get(), ZStreamPtr, Frame, [&]() {
    return inflateInit2(HostZStream.get(), WindowBits);
  });

  if (ZRes == Z_OK)
    Env.ZStreamMap.emplace(std::make_pair(ZStreamPtr, std::move(HostZStream)));

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
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Dictionary = MemInst->getPointer<Bytef *>(DictionaryPtr);

  const int32_t ZRes =
      SyncRun(HostZStreamIt->second.get(), ZStreamPtr, Frame, [&]() {
        return inflateSetDictionary(HostZStreamIt->second.get(), Dictionary,
                                    DictLength);
      });

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibInflateGetDictionary::body(
    const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
    uint32_t DictionaryPtr, uint32_t DictLengthPtr) {

  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

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
WasmEdgeZlibInflateBackInit::body(const Runtime::CallingFrame &Frame,
                                  uint32_t ZStreamPtr, int32_t WindowBits,
                                  uint32_t WindowPtr) {

  auto HostZStream = std::make_unique<z_stream>();
  HostZStream->zalloc = Z_NULL;
  HostZStream->zfree = Z_NULL;
  HostZStream->opaque =
      Z_NULL; // ignore opaque since zmalloc and zfree was ignored

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Window = MemInst->getPointer<unsigned char *>(WindowPtr);

  const int32_t ZRes = SyncRun(HostZStream.get(), ZStreamPtr, Frame, [&]() {
    return inflateBackInit(HostZStream.get(), WindowBits, Window);
  });

  if (ZRes == Z_OK)
    Env.ZStreamMap.emplace(std::make_pair(ZStreamPtr, std::move(HostZStream)));

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

Expect<int32_t>
WasmEdgeZlibZlibCompilerFlags::body(const Runtime::CallingFrame &) {

  const int32_t ZRes = zlibCompileFlags();
  return ZRes;
}

Expect<int32_t> WasmEdgeZlibCompress::body(const Runtime::CallingFrame &Frame,
                                           uint32_t DestPtr,
                                           uint32_t DestLenPtr,
                                           uint32_t SourcePtr,
                                           uint32_t SourceLen) {

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Dest = MemInst->getPointer<Bytef *>(DestPtr);
  auto *DestLen = MemInst->getPointer<uint32_t *>(DestLenPtr);
  auto *Source = MemInst->getPointer<Bytef *>(SourcePtr);

  unsigned long HostDestLen;
  HostDestLen = *DestLen;
  const int32_t ZRes = compress(Dest, &HostDestLen, Source, SourceLen);
  *DestLen = HostDestLen;

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibCompress2::body(const Runtime::CallingFrame &Frame,
                                            uint32_t DestPtr,
                                            uint32_t DestLenPtr,
                                            uint32_t SourcePtr,
                                            uint32_t SourceLen, int32_t Level) {

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Dest = MemInst->getPointer<Bytef *>(DestPtr);
  auto *DestLen = MemInst->getPointer<uint32_t *>(DestLenPtr);
  auto *Source = MemInst->getPointer<Bytef *>(SourcePtr);

  unsigned long HostDestLen;
  HostDestLen = *DestLen;
  const int32_t ZRes = compress2(Dest, &HostDestLen, Source, SourceLen, Level);
  *DestLen = HostDestLen;

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibCompressBound::body(const Runtime::CallingFrame &,
                                                uint32_t SourceLen) {
  const int32_t ZRes = compressBound(SourceLen);

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibUncompress::body(const Runtime::CallingFrame &Frame,
                                             uint32_t DestPtr,
                                             uint32_t DestLenPtr,
                                             uint32_t SourcePtr,
                                             uint32_t SourceLen) {

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Dest = MemInst->getPointer<Bytef *>(DestPtr);
  auto *DestLen = MemInst->getPointer<uint32_t *>(DestLenPtr);
  auto *Source = MemInst->getPointer<Bytef *>(SourcePtr);

  unsigned long HostDestLen;
  HostDestLen = *DestLen;
  const int32_t ZRes = uncompress(Dest, &HostDestLen, Source, SourceLen);
  *DestLen = HostDestLen;

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibUncompress2::body(const Runtime::CallingFrame &Frame,
                              uint32_t DestPtr, uint32_t DestLenPtr,
                              uint32_t SourcePtr, uint32_t SourceLenPtr) {

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Dest = MemInst->getPointer<Bytef *>(DestPtr);
  auto *DestLen = MemInst->getPointer<uint32_t *>(DestLenPtr);
  auto *Source = MemInst->getPointer<Bytef *>(SourcePtr);
  auto *SourceLen = MemInst->getPointer<uint32_t *>(SourceLenPtr);

  unsigned long HostDestLen, HostSourceLen;
  HostDestLen = *DestLen;
  HostSourceLen = *SourceLen;
  const int32_t ZRes = uncompress2(Dest, &HostDestLen, Source, &HostSourceLen);
  *DestLen = HostDestLen;
  *SourceLen = HostSourceLen;

  return ZRes;
}

Expect<uint32_t> WasmEdgeZlibGZOpen::body(const Runtime::CallingFrame &Frame,
                                          uint32_t PathPtr, uint32_t ModePtr) {

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Path = MemInst->getPointer<const char *>(PathPtr);
  auto *Mode = MemInst->getPointer<const char *>(ModePtr);

  auto ZRes = gzopen(Path, Mode);

  const auto NewWasmGZFile = WasmGZFileStart + Env.GZFileMap.size();
  auto El =
      std::pair<uint32_t, std::unique_ptr<WasmEdgeZlibEnvironment::GZFile_s>>(
          NewWasmGZFile, ZRes);

  Env.GZFileMap.emplace(std::move(El));

  return 0;
}

Expect<uint32_t> WasmEdgeZlibGZDOpen::body(const Runtime::CallingFrame &Frame,
                                           int32_t FD, uint32_t ModePtr) {

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Mode = MemInst->getPointer<const char *>(ModePtr);

  auto ZRes = gzdopen(FD, Mode);

  const auto NewWasmGZFile = WasmGZFileStart + Env.GZFileMap.size();
  auto El =
      std::pair<uint32_t, std::unique_ptr<WasmEdgeZlibEnvironment::GZFile_s>>(
          NewWasmGZFile, ZRes);

  Env.GZFileMap.emplace(std::move(El));

  return 0;
}

Expect<int32_t> WasmEdgeZlibGZBuffer::body(const Runtime::CallingFrame &,
                                           uint32_t GZFile, uint32_t Size) {

  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto ZRes = gzbuffer(GZFileIt->second.get(), Size);

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibGZSetParams::body(const Runtime::CallingFrame &,
                                              uint32_t GZFile, int32_t Level,
                                              int32_t Strategy) {

  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto ZRes = gzsetparams(GZFileIt->second.get(), Level, Strategy);

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibGZRead::body(const Runtime::CallingFrame &Frame,
                                         uint32_t GZFile, uint32_t BufPtr,
                                         uint32_t Len) {

  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Buf = MemInst->getPointer<unsigned char *>(BufPtr);

  auto ZRes = gzread(GZFileIt->second.get(), Buf, Len);

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibGZFread::body(const Runtime::CallingFrame &Frame,
                                          uint32_t BufPtr, uint32_t Size,
                                          uint32_t NItems, uint32_t GZFile) {

  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Buf = MemInst->getPointer<unsigned char *>(BufPtr);

  auto ZRes = gzfread(Buf, Size, NItems, GZFileIt->second.get());

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibGZWrite::body(const Runtime::CallingFrame &Frame,
                                          uint32_t GZFile, uint32_t BufPtr,
                                          uint32_t Len) {

  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Buf = MemInst->getPointer<unsigned char *>(BufPtr);

  auto ZRes = gzwrite(GZFileIt->second.get(), Buf, Len);

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibGZFwrite::body(const Runtime::CallingFrame &Frame,
                                           uint32_t BufPtr, uint32_t Size,
                                           uint32_t NItems, uint32_t GZFile) {

  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Buf = MemInst->getPointer<unsigned char *>(BufPtr);

  auto ZRes = gzfwrite(Buf, Size, NItems, GZFileIt->second.get());

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibGZPuts::body(const Runtime::CallingFrame &Frame,
                                         uint32_t GZFile, uint32_t StringPtr) {

  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *String = MemInst->getPointer<const char *>(StringPtr);

  auto ZRes = gzputs(GZFileIt->second.get(), String);

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibGZPutc::body(const Runtime::CallingFrame &,
                                         uint32_t GZFile, int32_t C) {

  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto ZRes = gzputc(GZFileIt->second.get(), C);

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibGZGetc::body(const Runtime::CallingFrame &,
                                         uint32_t GZFile) {

  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto ZRes = gzgetc(GZFileIt->second.get());

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibGZUngetc::body(const Runtime::CallingFrame &,
                                           int32_t C, uint32_t GZFile) {

  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto ZRes = gzungetc(C, GZFileIt->second.get());

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibGZFlush::body(const Runtime::CallingFrame &,
                                          uint32_t GZFile, int32_t Flush) {

  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto ZRes = gzflush(GZFileIt->second.get(), Flush);

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibGZSeek::body(const Runtime::CallingFrame &,
                                         uint32_t GZFile, int32_t Offset,
                                         int32_t Whence) {

  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto ZRes = gzseek(GZFileIt->second.get(), Offset, Whence);

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibGZRewind::body(const Runtime::CallingFrame &,
                                           uint32_t GZFile) {

  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto ZRes = gzrewind(GZFileIt->second.get());

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibGZTell::body(const Runtime::CallingFrame &,
                                         uint32_t GZFile) {

  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto ZRes = gztell(GZFileIt->second.get());

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibGZOffset::body(const Runtime::CallingFrame &,
                                           uint32_t GZFile) {

  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto ZRes = gzoffset(GZFileIt->second.get());

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibGZEof::body(const Runtime::CallingFrame &,
                                        uint32_t GZFile) {

  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto ZRes = gzeof(GZFileIt->second.get());

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibGZDirect::body(const Runtime::CallingFrame &,
                                           uint32_t GZFile) {

  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto ZRes = gzdirect(GZFileIt->second.get());

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibGZClose::body(const Runtime::CallingFrame &,
                                          uint32_t GZFile) {

  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto ZRes = gzclose(GZFileIt->second.get());

  Env.GZFileMap.erase(GZFileIt);

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibGZClose_r::body(const Runtime::CallingFrame &,
                                            uint32_t GZFile) {

  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto ZRes = gzclose_r(GZFileIt->second.get());

  Env.GZFileMap.erase(GZFileIt);

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibGZClose_w::body(const Runtime::CallingFrame &,
                                            uint32_t GZFile) {

  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto ZRes = gzclose_w(GZFileIt->second.get());

  Env.GZFileMap.erase(GZFileIt);

  return ZRes;
}

Expect<void> WasmEdgeZlibGZClearerr::body(const Runtime::CallingFrame &,
                                          uint32_t GZFile) {

  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  gzclearerr(GZFileIt->second.get());

  return Expect<void>{};
}

Expect<int32_t> WasmEdgeZlibAdler32::body(const Runtime::CallingFrame &Frame,
                                          uint32_t Adler, uint32_t BufPtr,
                                          uint32_t Len) {

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Buf = MemInst->getPointer<Bytef *>(BufPtr);

  const int32_t ZRes = adler32(Adler, Buf, Len);
  return ZRes;
}

Expect<int32_t> WasmEdgeZlibAdler32_z::body(const Runtime::CallingFrame &Frame,
                                            uint32_t Adler, uint32_t BufPtr,
                                            uint32_t Len) {

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Buf = MemInst->getPointer<Bytef *>(BufPtr);

  const int32_t ZRes = adler32_z(Adler, Buf, Len);
  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibAdler32Combine::body(const Runtime::CallingFrame &Frame,
                                 uint32_t Adler1, uint32_t Adler2,
                                 int32_t Len2) {

  const int32_t ZRes = adler32_combine(Adler1, Adler2, Len2);
  return ZRes;
}

Expect<int32_t> WasmEdgeZlibCRC32::body(const Runtime::CallingFrame &Frame,
                                        uint32_t CRC, uint32_t BufPtr,
                                        uint32_t Len) {

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Buf = MemInst->getPointer<Bytef *>(BufPtr);

  const int32_t ZRes = crc32(CRC, Buf, Len);

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibCRC32_z::body(const Runtime::CallingFrame &Frame,
                                          uint32_t CRC, uint32_t BufPtr,
                                          uint32_t Len) {

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Buf = MemInst->getPointer<Bytef *>(BufPtr);

  const int32_t ZRes = crc32_z(CRC, Buf, Len);

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibCRC32Combine::body(const Runtime::CallingFrame &Frame,
                               uint32_t CRC1, uint32_t CRC2, int32_t Len2) {

  const int32_t ZRes = crc32_combine(CRC1, CRC2, Len2);
  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibDeflateInit_::body(const Runtime::CallingFrame &Frame,
                               uint32_t ZStreamPtr, int32_t Level,
                               uint32_t VersionPtr, int32_t StreamSize) {

  if (!CheckSize(StreamSize))
    return static_cast<int32_t>(Z_VERSION_ERROR);

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const auto *WasmZlibVersion = MemInst->getPointer<const char *>(VersionPtr);
  auto HostZStream = std::make_unique<z_stream>();

  // ignore wasm custom allocators
  HostZStream->zalloc = Z_NULL;
  HostZStream->zfree = Z_NULL;
  // ignore opaque since zmalloc and zfree was ignored
  HostZStream->opaque = Z_NULL;

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

  if (!CheckSize(StreamSize))
    return static_cast<int32_t>(Z_VERSION_ERROR);

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const auto *WasmZlibVersion = MemInst->getPointer<const char *>(VersionPtr);
  auto HostZStream = std::make_unique<z_stream>();

  // ignore wasm custom allocators
  HostZStream->zalloc = Z_NULL;
  HostZStream->zfree = Z_NULL;
  // ignore opaque since zmalloc and zfree was ignored
  HostZStream->opaque = Z_NULL;

  const int32_t ZRes = SyncRun(HostZStream.get(), ZStreamPtr, Frame, [&]() {
    return inflateInit_(HostZStream.get(), WasmZlibVersion, sizeof(z_stream));
  });

  if (ZRes == Z_OK)
    Env.ZStreamMap.emplace(std::make_pair(ZStreamPtr, std::move(HostZStream)));

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibDeflateInit2_::body(
    const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr, int32_t Level,
    int32_t Method, int32_t WindowBits, int32_t MemLevel, int32_t Strategy,
    uint32_t VersionPtr, int32_t StreamSize) {

  if (!CheckSize(StreamSize))
    return static_cast<int32_t>(Z_VERSION_ERROR);

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const auto *WasmZlibVersion = MemInst->getPointer<const char *>(VersionPtr);
  auto HostZStream = std::make_unique<z_stream>();
  HostZStream->zalloc = Z_NULL;
  HostZStream->zfree = Z_NULL;
  HostZStream->opaque =
      Z_NULL; // ignore opaque since zmalloc and zfree was ignored

  const int32_t ZRes = SyncRun(HostZStream.get(), ZStreamPtr, Frame, [&]() {
    return deflateInit2_(HostZStream.get(), Level, Method, WindowBits, MemLevel,
                         Strategy, WasmZlibVersion, sizeof(z_stream));
  });

  if (ZRes == Z_OK)
    Env.ZStreamMap.emplace(std::make_pair(ZStreamPtr, std::move(HostZStream)));

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibInflateInit2_::body(const Runtime::CallingFrame &Frame,
                                uint32_t ZStreamPtr, int32_t WindowBits,
                                uint32_t VersionPtr, int32_t StreamSize) {

  if (!CheckSize(StreamSize))
    return static_cast<int32_t>(Z_VERSION_ERROR);

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const auto *WasmZlibVersion = MemInst->getPointer<const char *>(VersionPtr);
  auto HostZStream = std::make_unique<z_stream>();
  HostZStream->zalloc = Z_NULL;
  HostZStream->zfree = Z_NULL;
  HostZStream->opaque =
      Z_NULL; // ignore opaque since zmalloc and zfree was ignored

  const int32_t ZRes = SyncRun(HostZStream.get(), ZStreamPtr, Frame, [&]() {
    return inflateInit2_(HostZStream.get(), WindowBits, WasmZlibVersion,
                         sizeof(z_stream));
  });

  if (ZRes == Z_OK)
    Env.ZStreamMap.emplace(std::make_pair(ZStreamPtr, std::move(HostZStream)));

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibInflateBackInit_::body(
    const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr, int32_t WindowBits,
    uint32_t WindowPtr, uint32_t VersionPtr, int32_t StreamSize) {

  if (!CheckSize(StreamSize))
    return static_cast<int32_t>(Z_VERSION_ERROR);

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const auto *WasmZlibVersion = MemInst->getPointer<const char *>(VersionPtr);
  auto *Window = MemInst->getPointer<unsigned char *>(WindowPtr);
  auto HostZStream = std::make_unique<z_stream>();
  HostZStream->zalloc = Z_NULL;
  HostZStream->zfree = Z_NULL;
  HostZStream->opaque =
      Z_NULL; // ignore opaque since zmalloc and zfree was ignored

  const int32_t ZRes = SyncRun(HostZStream.get(), ZStreamPtr, Frame, [&]() {
    return inflateBackInit_(HostZStream.get(), WindowBits, Window,
                            WasmZlibVersion, sizeof(z_stream));
  });

  if (ZRes == Z_OK)
    Env.ZStreamMap.emplace(std::make_pair(ZStreamPtr, std::move(HostZStream)));

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibInflateSyncPoint::body(const Runtime::CallingFrame &,
                                   uint32_t ZStreamPtr) {

  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto ZRes = inflateSyncPoint(HostZStreamIt->second.get());

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibInflateUndermine::body(const Runtime::CallingFrame &,
                                   uint32_t ZStreamPtr, int32_t Subvert) {

  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto ZRes = inflateUndermine(HostZStreamIt->second.get(), Subvert);

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibInflateValidate::body(const Runtime::CallingFrame &,
                                                  uint32_t ZStreamPtr,
                                                  int32_t Check) {

  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto ZRes = inflateValidate(HostZStreamIt->second.get(), Check);

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibInflateCodesUsed::body(const Runtime::CallingFrame &,
                                   uint32_t ZStreamPtr) {

  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto ZRes = inflateCodesUsed(HostZStreamIt->second.get());

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibInflateResetKeep::body(const Runtime::CallingFrame &,
                                   uint32_t ZStreamPtr) {

  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto ZRes = inflateResetKeep(HostZStreamIt->second.get());

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibDeflateResetKeep::body(const Runtime::CallingFrame &,
                                   uint32_t ZStreamPtr) {

  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto ZRes = deflateResetKeep(HostZStreamIt->second.get());

  return ZRes;
}

} // namespace Host
} // namespace WasmEdge
