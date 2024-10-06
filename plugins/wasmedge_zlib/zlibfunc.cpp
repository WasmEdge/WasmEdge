// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "zlibfunc.h"

#include <cstring>

namespace WasmEdge {
namespace Host {

constexpr bool CheckSize(int32_t StreamSize) {

  return (StreamSize == static_cast<int32_t>(sizeof(WasmZStream)));
}

static constexpr uint32_t WasmGZFileStart = sizeof(gzFile);

template <typename T>
auto SyncRun(const std::string_view &Msg, WasmEdgeZlibEnvironment &Env,
             uint32_t ZStreamPtr, const Runtime::CallingFrame &Frame,
             T Callback) -> Expect<int32_t> {

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    spdlog::error("[WasmEdge-Zlib] [{}-SyncRun] "sv
                  "Frame.getMemoryByIndex(0) returned nullptr."sv,
                  Msg);
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  WasmZStream *ModuleZStream = MemInst->getPointer<WasmZStream *>(ZStreamPtr);

  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [{}-SyncRun] "sv
                  "Invalid ZStreamPtr received."sv,
                  Msg);
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  auto HostZStream = HostZStreamIt->second.get();
  const auto GZHeaderStoreIt = Env.GZHeaderMap.find(ZStreamPtr);

  HostZStream->next_in =
      MemInst->getPointer<unsigned char *>(ModuleZStream->NextIn);
  HostZStream->avail_in = ModuleZStream->AvailIn;
  HostZStream->total_in = ModuleZStream->TotalIn;

  HostZStream->next_out =
      MemInst->getPointer<unsigned char *>(ModuleZStream->NextOut);
  HostZStream->avail_out = ModuleZStream->AvailOut;
  HostZStream->total_out = ModuleZStream->TotalOut;

  // TODO: ignore msg for now
  // ignore state
  // ignore zalloc, zfree, opaque

  HostZStream->data_type = ModuleZStream->DataType;
  HostZStream->adler = ModuleZStream->Adler;
  HostZStream->reserved = ModuleZStream->Reserved;

  const auto PreComputeNextIn = HostZStream->next_in;
  const auto PreComputeNextOut = HostZStream->next_out;

  unsigned char *PreComputeExtra{};
  unsigned char *PreComputeName{};
  unsigned char *PreComputeComment{};

  if (GZHeaderStoreIt != Env.GZHeaderMap.end()) {
    // Sync GZ Header

    auto *ModuleGZHeader = MemInst->getPointer<WasmGZHeader *>(
        GZHeaderStoreIt->second.WasmGZHeaderOffset);
    auto *HostGZHeader = GZHeaderStoreIt->second.HostGZHeader.get();

    HostGZHeader->text = ModuleGZHeader->Text;
    HostGZHeader->time = ModuleGZHeader->Time;
    HostGZHeader->xflags = ModuleGZHeader->XFlags;
    HostGZHeader->os = ModuleGZHeader->OS;

    HostGZHeader->extra =
        MemInst->getPointer<unsigned char *>(ModuleGZHeader->Extra);
    HostGZHeader->extra_len = ModuleGZHeader->ExtraLen;
    HostGZHeader->extra_max = ModuleGZHeader->ExtraMax;

    HostGZHeader->name =
        MemInst->getPointer<unsigned char *>(ModuleGZHeader->Name);
    HostGZHeader->name_max = ModuleGZHeader->NameMax;

    HostGZHeader->comment =
        MemInst->getPointer<unsigned char *>(ModuleGZHeader->Comment);
    HostGZHeader->comm_max = ModuleGZHeader->CommMax;

    HostGZHeader->hcrc = ModuleGZHeader->HCRC;
    HostGZHeader->done = ModuleGZHeader->Done;

    PreComputeExtra = HostGZHeader->extra;
    PreComputeName = HostGZHeader->name;
    PreComputeComment = HostGZHeader->comment;
  }

  const auto ZRes = Callback(HostZStream);

  ModuleZStream->NextIn += HostZStream->next_in - PreComputeNextIn;
  ModuleZStream->AvailIn = HostZStream->avail_in;
  ModuleZStream->TotalIn = HostZStream->total_in;

  ModuleZStream->NextOut += HostZStream->next_out - PreComputeNextOut;
  ModuleZStream->AvailOut = HostZStream->avail_out;
  ModuleZStream->TotalOut = HostZStream->total_out;

  // TODO: ignore msg for now
  // ignore state
  // ignore zalloc, zfree, opaque

  ModuleZStream->DataType = HostZStream->data_type;
  ModuleZStream->Adler = HostZStream->adler;
  ModuleZStream->Reserved = HostZStream->reserved;

  if (GZHeaderStoreIt != Env.GZHeaderMap.end()) {
    // Sync GZ Header

    auto *ModuleGZHeader = MemInst->getPointer<WasmGZHeader *>(
        GZHeaderStoreIt->second.WasmGZHeaderOffset);
    auto *HostGZHeader = GZHeaderStoreIt->second.HostGZHeader.get();

    ModuleGZHeader->Text = HostGZHeader->text;
    ModuleGZHeader->Time = HostGZHeader->time;
    ModuleGZHeader->XFlags = HostGZHeader->xflags;
    ModuleGZHeader->OS = HostGZHeader->os;

    ModuleGZHeader->Extra += HostGZHeader->extra - PreComputeExtra;
    ModuleGZHeader->ExtraLen = HostGZHeader->extra_len;
    ModuleGZHeader->ExtraMax = HostGZHeader->extra_max;

    ModuleGZHeader->Name += HostGZHeader->name - PreComputeName;
    ModuleGZHeader->NameMax = HostGZHeader->name_max;

    ModuleGZHeader->Comment += HostGZHeader->comment - PreComputeComment;
    ModuleGZHeader->CommMax = HostGZHeader->comm_max;

    ModuleGZHeader->HCRC = HostGZHeader->hcrc;
    ModuleGZHeader->Done = HostGZHeader->done;
  }

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

  auto It =
      Env.ZStreamMap.emplace(std::make_pair(ZStreamPtr, std::move(HostZStream)))
          .second;

  const auto ZRes = SyncRun(
      "WasmEdgeZlibDeflateInit", Env, ZStreamPtr, Frame,
      [&](z_stream *HostZStream) { return deflateInit(HostZStream, Level); });

  if (ZRes != Z_OK)
    Env.ZStreamMap.erase(It);

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibDeflate::WasmEdgeZlibDeflate::body(
    const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr, int32_t Flush) {

  return SyncRun(
      "WasmEdgeZlibDeflate", Env, ZStreamPtr, Frame,
      [&](z_stream *HostZStream) { return deflate(HostZStream, Flush); });
}

Expect<int32_t> WasmEdgeZlibDeflateEnd::body(const Runtime::CallingFrame &Frame,
                                             uint32_t ZStreamPtr) {

  const auto ZRes =
      SyncRun("WasmEdgeZlibDeflateEnd", Env, ZStreamPtr, Frame,
              [&](z_stream *HostZStream) { return deflateEnd(HostZStream); });

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

  auto It =
      Env.ZStreamMap.emplace(std::make_pair(ZStreamPtr, std::move(HostZStream)))
          .second;

  const auto ZRes =
      SyncRun("WasmEdgeZlibInflateInit", Env, ZStreamPtr, Frame,
              [&](z_stream *HostZStream) { return inflateInit(HostZStream); });

  if (ZRes != Z_OK)
    Env.ZStreamMap.erase(It);

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibInflate::body(const Runtime::CallingFrame &Frame,
                                          uint32_t ZStreamPtr, int32_t Flush) {

  return SyncRun(
      "WasmEdgeZlibInflate", Env, ZStreamPtr, Frame,
      [&](z_stream *HostZStream) { return inflate(HostZStream, Flush); });
}

Expect<int32_t> WasmEdgeZlibInflateEnd::body(const Runtime::CallingFrame &Frame,
                                             uint32_t ZStreamPtr) {

  const auto ZRes =
      SyncRun("WasmEdgeZlibInflateEnd", Env, ZStreamPtr, Frame,
              [&](z_stream *HostZStream) { return inflateEnd(HostZStream); });

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

  auto It =
      Env.ZStreamMap.emplace(std::make_pair(ZStreamPtr, std::move(HostZStream)))
          .second;

  const auto ZRes =
      SyncRun("WasmEdgeZlibDeflateInit2", Env, ZStreamPtr, Frame,
              [&](z_stream *HostZStream) {
                return deflateInit2(HostZStream, Level, Method, WindowBits,
                                    MemLevel, Strategy);
              });

  if (ZRes != Z_OK)
    Env.ZStreamMap.erase(It);

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibDeflateSetDictionary::body(
    const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
    uint32_t DictionaryPtr, uint32_t DictLength) {

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibDeflateSetDictionary] "sv
                  "Frame.getMemoryByIndex(0) returned nullptr."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const auto *Dictionary = MemInst->getPointer<const Bytef *>(DictionaryPtr);

  return SyncRun("WasmEdgeZlibDeflateSetDictionary", Env, ZStreamPtr, Frame,
                 [&](z_stream *HostZStream) {
                   return deflateSetDictionary(HostZStream, Dictionary,
                                               DictLength);
                 });
}

Expect<int32_t> WasmEdgeZlibDeflateGetDictionary::body(
    const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
    uint32_t DictionaryPtr, uint32_t DictLengthPtr) {

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibDeflateGetDictionary] "sv
                  "Frame.getMemoryByIndex(0) returned nullptr."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Dictionary = MemInst->getPointer<Bytef *>(DictionaryPtr);
  auto *DictLength = MemInst->getPointer<uint32_t *>(DictLengthPtr);

  return SyncRun("WasmEdgeZlibDeflateGetDictionary", Env, ZStreamPtr, Frame,
                 [&](z_stream *HostZStream) {
                   return deflateGetDictionary(HostZStream, Dictionary,
                                               DictLength);
                 });
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
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibDeflateCopy] "sv
                  "Invalid SourcePtr received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto DestZStream = std::make_unique<z_stream>();

  auto It =
      Env.ZStreamMap.emplace(std::make_pair(DestPtr, std::move(DestZStream)))
          .second;

  const auto Res = SyncRun("WasmEdgeZlibDeflateCopy", Env, DestPtr, Frame,
                           [&](z_stream *) { return 0; });
  if (!Res.has_value())
    return Res;

  const auto ZRes =
      SyncRun("WasmEdgeZlibDeflateCopy", Env, DestPtr, Frame,
              [&](z_stream *DestZStream) {
                return deflateCopy(DestZStream, SourceZStreamIt->second.get());
              });

  if (ZRes != Z_OK)
    Env.ZStreamMap.erase(It);

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibDeflateReset::body(const Runtime::CallingFrame &Frame,
                               uint32_t ZStreamPtr) {

  return SyncRun(
      "WasmEdgeZlibDeflateReset", Env, ZStreamPtr, Frame,
      [&](z_stream *HostZStream) { return deflateReset(HostZStream); });
}

Expect<int32_t>
WasmEdgeZlibDeflateParams::body(const Runtime::CallingFrame &Frame,
                                uint32_t ZStreamPtr, int32_t Level,
                                int32_t Strategy) {

  return SyncRun("WasmEdgeZlibDeflateParams", Env, ZStreamPtr, Frame,
                 [&](z_stream *HostZStream) {
                   return deflateParams(HostZStream, Level, Strategy);
                 });
}

Expect<int32_t> WasmEdgeZlibDeflateTune::body(
    const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr, int32_t GoodLength,
    int32_t MaxLazy, int32_t NiceLength, int32_t MaxChain) {

  return SyncRun("WasmEdgeZlibDeflateTune", Env, ZStreamPtr, Frame,
                 [&](z_stream *HostZStream) {
                   return deflateTune(HostZStream, GoodLength, MaxLazy,
                                      NiceLength, MaxChain);
                 });
}

Expect<int32_t>
WasmEdgeZlibDeflateBound::body(const Runtime::CallingFrame &Frame,
                               uint32_t ZStreamPtr, uint32_t SourceLen) {

  return SyncRun("WasmEdgeZlibDeflateBound", Env, ZStreamPtr, Frame,
                 [&](z_stream *HostZStream) {
                   return deflateBound(HostZStream, SourceLen);
                 });
}

Expect<int32_t>
WasmEdgeZlibDeflatePending::body(const Runtime::CallingFrame &Frame,
                                 uint32_t ZStreamPtr, uint32_t PendingPtr,
                                 uint32_t BitsPtr) {

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibDeflatePending] "sv
                  "Frame.getMemoryByIndex(0) returned nullptr."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Pending = MemInst->getPointer<uint32_t *>(PendingPtr);
  auto *Bits = MemInst->getPointer<int32_t *>(BitsPtr);

  return SyncRun("WasmEdgeZlibDeflatePending", Env, ZStreamPtr, Frame,
                 [&](z_stream *HostZStream) {
                   return deflatePending(HostZStream, Pending, Bits);
                 });
}

Expect<int32_t>
WasmEdgeZlibDeflatePrime::body(const Runtime::CallingFrame &Frame,
                               uint32_t ZStreamPtr, int32_t Bits,
                               int32_t Value) {

  return SyncRun("WasmEdgeZlibDeflatePrime", Env, ZStreamPtr, Frame,
                 [&](z_stream *HostZStream) {
                   return deflatePrime(HostZStream, Bits, Value);
                 });
}

Expect<int32_t>
WasmEdgeZlibDeflateSetHeader::body(const Runtime::CallingFrame &Frame,
                                   uint32_t ZStreamPtr, uint32_t HeadPtr) {

  auto HostGZHeader = std::make_unique<gz_header>();
  auto HostGZHeaderPtr = HostGZHeader.get();

  auto It = Env.GZHeaderMap
                .emplace(std::pair<uint32_t, WasmEdgeZlibEnvironment::GZStore>{
                    ZStreamPtr,
                    WasmEdgeZlibEnvironment::GZStore{
                        .WasmGZHeaderOffset = HeadPtr,
                        .HostGZHeader = std::move(HostGZHeader)}})
                .second;

  const auto ZRes =
      SyncRun("WasmEdgeZlibDeflateSetHeader", Env, ZStreamPtr, Frame,
              [&](z_stream *HostZStream) {
                return deflateSetHeader(HostZStream, HostGZHeaderPtr);
              });

  if (ZRes != Z_OK)
    Env.GZHeaderMap.erase(It);

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

  auto It =
      Env.ZStreamMap.emplace(std::make_pair(ZStreamPtr, std::move(HostZStream)))
          .second;

  const auto ZRes = SyncRun("WasmEdgeZlibInflateInit2", Env, ZStreamPtr, Frame,
                            [&](z_stream *HostZStream) {
                              return inflateInit2(HostZStream, WindowBits);
                            });

  if (ZRes != Z_OK)
    Env.ZStreamMap.erase(It);

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibInflateSetDictionary::body(
    const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
    uint32_t DictionaryPtr, uint32_t DictLength) {

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibInflateSetDictionary] "sv
                  "Frame.getMemoryByIndex(0) returned nullptr."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Dictionary = MemInst->getPointer<Bytef *>(DictionaryPtr);

  return SyncRun("WasmEdgeZlibInflateSetDictionary", Env, ZStreamPtr, Frame,
                 [&](z_stream *HostZStream) {
                   return inflateSetDictionary(HostZStream, Dictionary,
                                               DictLength);
                 });
}

Expect<int32_t> WasmEdgeZlibInflateGetDictionary::body(
    const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
    uint32_t DictionaryPtr, uint32_t DictLengthPtr) {

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibInflateGetDictionary] "sv
                  "Frame.getMemoryByIndex(0) returned nullptr."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Dictionary = MemInst->getPointer<Bytef *>(DictionaryPtr);
  auto *DictLength = MemInst->getPointer<uint32_t *>(DictLengthPtr);

  return SyncRun("WasmEdgeZlibInflateGetDictionary", Env, ZStreamPtr, Frame,
                 [&](z_stream *HostZStream) {
                   return inflateGetDictionary(HostZStream, Dictionary,
                                               DictLength);
                 });
}

Expect<int32_t>
WasmEdgeZlibInflateSync::body(const Runtime::CallingFrame &Frame,
                              uint32_t ZStreamPtr) {

  return SyncRun(
      "WasmEdgeZlibInflateSync", Env, ZStreamPtr, Frame,
      [&](z_stream *HostZStream) { return inflateSync(HostZStream); });
}

Expect<int32_t>
WasmEdgeZlibInflateCopy::body(const Runtime::CallingFrame &Frame,
                              uint32_t DestPtr, uint32_t SourcePtr) {
  const auto SourceZStreamIt = Env.ZStreamMap.find(SourcePtr);
  if (SourceZStreamIt == Env.ZStreamMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibInflateCopy] "sv
                  "Invalid SourcePtr received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto DestZStream = std::make_unique<z_stream>();

  auto It =
      Env.ZStreamMap.emplace(std::make_pair(DestPtr, std::move(DestZStream)))
          .second;

  const auto Res = SyncRun("WasmEdgeZlibInflateCopy", Env, DestPtr, Frame,
                           [&](z_stream *) { return 0; });
  if (!Res.has_value())
    return Res;

  const auto ZRes =
      SyncRun("WasmEdgeZlibInflateCopy", Env, DestPtr, Frame,
              [&](z_stream *DestZStream) {
                return inflateCopy(DestZStream, SourceZStreamIt->second.get());
              });

  if (ZRes != Z_OK)
    Env.ZStreamMap.erase(It);

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibInflateReset::body(const Runtime::CallingFrame &Frame,
                               uint32_t ZStreamPtr) {

  return SyncRun(
      "WasmEdgeZlibInflateReset", Env, ZStreamPtr, Frame,
      [&](z_stream *HostZStream) { return inflateReset(HostZStream); });
}

Expect<int32_t>
WasmEdgeZlibInflateReset2::body(const Runtime::CallingFrame &Frame,
                                uint32_t ZStreamPtr, int32_t WindowBits) {

  return SyncRun("WasmEdgeZlibInflateReset2", Env, ZStreamPtr, Frame,
                 [&](z_stream *HostZStream) {
                   return inflateReset2(HostZStream, WindowBits);
                 });
}

Expect<int32_t>
WasmEdgeZlibInflatePrime::body(const Runtime::CallingFrame &Frame,
                               uint32_t ZStreamPtr, int32_t Bits,
                               int32_t Value) {

  return SyncRun("WasmEdgeZlibInflatePrime", Env, ZStreamPtr, Frame,
                 [&](z_stream *HostZStream) {
                   return inflatePrime(HostZStream, Bits, Value);
                 });
}

Expect<int32_t>
WasmEdgeZlibInflateMark::body(const Runtime::CallingFrame &Frame,
                              uint32_t ZStreamPtr) {

  return SyncRun(
      "WasmEdgeZlibInflateMark", Env, ZStreamPtr, Frame,
      [&](z_stream *HostZStream) { return inflateMark(HostZStream); });
}

Expect<int32_t>
WasmEdgeZlibInflateGetHeader::body(const Runtime::CallingFrame &Frame,
                                   uint32_t ZStreamPtr, uint32_t HeadPtr) {

  auto HostGZHeader = std::make_unique<gz_header>();
  auto HostGZHeaderPtr = HostGZHeader.get();

  auto It = Env.GZHeaderMap
                .emplace(std::pair<uint32_t, WasmEdgeZlibEnvironment::GZStore>{
                    ZStreamPtr,
                    WasmEdgeZlibEnvironment::GZStore{
                        .WasmGZHeaderOffset = HeadPtr,
                        .HostGZHeader = std::move(HostGZHeader)}})
                .second;

  const auto ZRes =
      SyncRun("WasmEdgeZlibInflateGetHeader", Env, ZStreamPtr, Frame,
              [&](z_stream *HostZStream) {
                return inflateGetHeader(HostZStream, HostGZHeaderPtr);
              });

  if (ZRes != Z_OK)
    Env.GZHeaderMap.erase(It);

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
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibInflateBackInit] "sv
                  "Frame.getMemoryByIndex(0) returned nullptr."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Window = MemInst->getPointer<unsigned char *>(WindowPtr);

  auto It =
      Env.ZStreamMap.emplace(std::make_pair(ZStreamPtr, std::move(HostZStream)))
          .second;

  const auto ZRes =
      SyncRun("WasmEdgeZlibInflateBackInit", Env, ZStreamPtr, Frame,
              [&](z_stream *HostZStream) {
                return inflateBackInit(HostZStream, WindowBits, Window);
              });

  if (ZRes != Z_OK)
    Env.ZStreamMap.erase(It);

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibInflateBackEnd::body(const Runtime::CallingFrame &Frame,
                                 uint32_t ZStreamPtr) {

  const auto ZRes = SyncRun(
      "WasmEdgeZlibInflateBackEnd", Env, ZStreamPtr, Frame,
      [&](z_stream *HostZStream) { return inflateBackEnd(HostZStream); });

  Env.ZStreamMap.erase(ZStreamPtr);

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibZlibCompilerFlags::body(const Runtime::CallingFrame &) {
  return zlibCompileFlags();
}

Expect<int32_t> WasmEdgeZlibCompress::body(const Runtime::CallingFrame &Frame,
                                           uint32_t DestPtr,
                                           uint32_t DestLenPtr,
                                           uint32_t SourcePtr,
                                           uint32_t SourceLen) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibCompress] "sv
                  "Frame.getMemoryByIndex(0) returned nullptr."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Dest = MemInst->getPointer<Bytef *>(DestPtr);
  auto *DestLen = MemInst->getPointer<uint32_t *>(DestLenPtr);
  auto *Source = MemInst->getPointer<Bytef *>(SourcePtr);

  unsigned long HostDestLen;
  HostDestLen = *DestLen;
  const auto ZRes = compress(Dest, &HostDestLen, Source, SourceLen);
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
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibCompress2] "sv
                  "Frame.getMemoryByIndex(0) returned nullptr."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Dest = MemInst->getPointer<Bytef *>(DestPtr);
  auto *DestLen = MemInst->getPointer<uint32_t *>(DestLenPtr);
  auto *Source = MemInst->getPointer<Bytef *>(SourcePtr);

  unsigned long HostDestLen;
  HostDestLen = *DestLen;
  const auto ZRes = compress2(Dest, &HostDestLen, Source, SourceLen, Level);
  *DestLen = HostDestLen;

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibCompressBound::body(const Runtime::CallingFrame &,
                                                uint32_t SourceLen) {
  return compressBound(SourceLen);
}

Expect<int32_t> WasmEdgeZlibUncompress::body(const Runtime::CallingFrame &Frame,
                                             uint32_t DestPtr,
                                             uint32_t DestLenPtr,
                                             uint32_t SourcePtr,
                                             uint32_t SourceLen) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibUncompress] "sv
                  "Frame.getMemoryByIndex(0) returned nullptr."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Dest = MemInst->getPointer<Bytef *>(DestPtr);
  auto *DestLen = MemInst->getPointer<uint32_t *>(DestLenPtr);
  auto *Source = MemInst->getPointer<Bytef *>(SourcePtr);

  unsigned long HostDestLen;
  HostDestLen = *DestLen;
  const auto ZRes = uncompress(Dest, &HostDestLen, Source, SourceLen);
  *DestLen = HostDestLen;

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibUncompress2::body(const Runtime::CallingFrame &Frame,
                              uint32_t DestPtr, uint32_t DestLenPtr,
                              uint32_t SourcePtr, uint32_t SourceLenPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibUncompress2] "sv
                  "Frame.getMemoryByIndex(0) returned nullptr."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Dest = MemInst->getPointer<Bytef *>(DestPtr);
  auto *DestLen = MemInst->getPointer<uint32_t *>(DestLenPtr);
  auto *Source = MemInst->getPointer<Bytef *>(SourcePtr);
  auto *SourceLen = MemInst->getPointer<uint32_t *>(SourceLenPtr);

  unsigned long HostDestLen, HostSourceLen;
  HostDestLen = *DestLen;
  HostSourceLen = *SourceLen;
  const auto ZRes = uncompress2(Dest, &HostDestLen, Source, &HostSourceLen);
  *DestLen = HostDestLen;
  *SourceLen = HostSourceLen;

  return ZRes;
}

Expect<uint32_t> WasmEdgeZlibGZOpen::body(const Runtime::CallingFrame &Frame,
                                          uint32_t PathPtr, uint32_t ModePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZOpen] "sv
                  "Frame.getMemoryByIndex(0) returned nullptr."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Path = MemInst->getPointer<const char *>(PathPtr);
  auto *Mode = MemInst->getPointer<const char *>(ModePtr);

  auto ZRes = gzopen(Path, Mode);

  const auto NewWasmGZFile = WasmGZFileStart + Env.GZFileMap.size();
  auto El =
      std::pair<uint32_t, std::unique_ptr<WasmEdgeZlibEnvironment::GZFile>>(
          NewWasmGZFile, ZRes);

  Env.GZFileMap.emplace(std::move(El));

  return NewWasmGZFile;
}

Expect<uint32_t> WasmEdgeZlibGZDOpen::body(const Runtime::CallingFrame &Frame,
                                           int32_t FD, uint32_t ModePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZDOpen] "sv
                  "Frame.getMemoryByIndex(0) returned nullptr."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Mode = MemInst->getPointer<const char *>(ModePtr);

  auto ZRes = gzdopen(FD, Mode);

  const auto NewWasmGZFile = WasmGZFileStart + Env.GZFileMap.size();
  auto El =
      std::pair<uint32_t, std::unique_ptr<WasmEdgeZlibEnvironment::GZFile>>(
          NewWasmGZFile, ZRes);

  Env.GZFileMap.emplace(std::move(El));

  return NewWasmGZFile;
}

Expect<int32_t> WasmEdgeZlibGZBuffer::body(const Runtime::CallingFrame &,
                                           uint32_t GZFile, uint32_t Size) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZBuffer] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return gzbuffer(GZFileIt->second.get(), Size);
}

Expect<int32_t> WasmEdgeZlibGZSetParams::body(const Runtime::CallingFrame &,
                                              uint32_t GZFile, int32_t Level,
                                              int32_t Strategy) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZSetParams] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return gzsetparams(GZFileIt->second.get(), Level, Strategy);
}

Expect<int32_t> WasmEdgeZlibGZRead::body(const Runtime::CallingFrame &Frame,
                                         uint32_t GZFile, uint32_t BufPtr,
                                         uint32_t Len) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZRead] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZRead] "sv
                  "Frame.getMemoryByIndex(0) returned nullptr."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Buf = MemInst->getPointer<unsigned char *>(BufPtr);

  return gzread(GZFileIt->second.get(), Buf, Len);
}

Expect<int32_t> WasmEdgeZlibGZFread::body(const Runtime::CallingFrame &Frame,
                                          uint32_t BufPtr, uint32_t Size,
                                          uint32_t NItems, uint32_t GZFile) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZFread] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZFread] "sv
                  "Frame.getMemoryByIndex(0) returned nullptr."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Buf = MemInst->getPointer<unsigned char *>(BufPtr);

  return gzfread(Buf, Size, NItems, GZFileIt->second.get());
}

Expect<int32_t> WasmEdgeZlibGZWrite::body(const Runtime::CallingFrame &Frame,
                                          uint32_t GZFile, uint32_t BufPtr,
                                          uint32_t Len) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZWrite] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZWrite] "sv
                  "Frame.getMemoryByIndex(0) returned nullptr."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Buf = MemInst->getPointer<unsigned char *>(BufPtr);

  return gzwrite(GZFileIt->second.get(), Buf, Len);
}

Expect<int32_t> WasmEdgeZlibGZFwrite::body(const Runtime::CallingFrame &Frame,
                                           uint32_t BufPtr, uint32_t Size,
                                           uint32_t NItems, uint32_t GZFile) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZFwrite] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZFwrite] "sv
                  "Frame.getMemoryByIndex(0) returned nullptr."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Buf = MemInst->getPointer<unsigned char *>(BufPtr);

  return gzfwrite(Buf, Size, NItems, GZFileIt->second.get());
}

Expect<int32_t> WasmEdgeZlibGZPuts::body(const Runtime::CallingFrame &Frame,
                                         uint32_t GZFile, uint32_t StringPtr) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZPuts] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZPuts] "sv
                  "Frame.getMemoryByIndex(0) returned nullptr."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *String = MemInst->getPointer<const char *>(StringPtr);

  return gzputs(GZFileIt->second.get(), String);
}

Expect<int32_t> WasmEdgeZlibGZPutc::body(const Runtime::CallingFrame &,
                                         uint32_t GZFile, int32_t C) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZPutc] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return gzputc(GZFileIt->second.get(), C);
}

Expect<int32_t> WasmEdgeZlibGZGetc::body(const Runtime::CallingFrame &,
                                         uint32_t GZFile) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZGetc] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return gzgetc(GZFileIt->second.get());
}

Expect<int32_t> WasmEdgeZlibGZUngetc::body(const Runtime::CallingFrame &,
                                           int32_t C, uint32_t GZFile) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZUngetc] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return gzungetc(C, GZFileIt->second.get());
}

Expect<int32_t> WasmEdgeZlibGZFlush::body(const Runtime::CallingFrame &,
                                          uint32_t GZFile, int32_t Flush) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZFlush] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return gzflush(GZFileIt->second.get(), Flush);
}

Expect<int32_t> WasmEdgeZlibGZSeek::body(const Runtime::CallingFrame &,
                                         uint32_t GZFile, int32_t Offset,
                                         int32_t Whence) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZSeek] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return gzseek(GZFileIt->second.get(), Offset, Whence);
}

Expect<int32_t> WasmEdgeZlibGZRewind::body(const Runtime::CallingFrame &,
                                           uint32_t GZFile) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZRewind] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return gzrewind(GZFileIt->second.get());
}

Expect<int32_t> WasmEdgeZlibGZTell::body(const Runtime::CallingFrame &,
                                         uint32_t GZFile) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZTell] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return gztell(GZFileIt->second.get());
}

Expect<int32_t> WasmEdgeZlibGZOffset::body(const Runtime::CallingFrame &,
                                           uint32_t GZFile) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZOffset] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return gzoffset(GZFileIt->second.get());
}

Expect<int32_t> WasmEdgeZlibGZEof::body(const Runtime::CallingFrame &,
                                        uint32_t GZFile) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZEof] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return gzeof(GZFileIt->second.get());
}

Expect<int32_t> WasmEdgeZlibGZDirect::body(const Runtime::CallingFrame &,
                                           uint32_t GZFile) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZDirect] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return gzdirect(GZFileIt->second.get());
}

Expect<int32_t> WasmEdgeZlibGZClose::body(const Runtime::CallingFrame &,
                                          uint32_t GZFile) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZClose] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const auto ZRes = gzclose(GZFileIt->second.get());

  Env.GZFileMap.erase(GZFileIt);

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibGZClose_r::body(const Runtime::CallingFrame &,
                                            uint32_t GZFile) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZClose_r] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const auto ZRes = gzclose_r(GZFileIt->second.get());

  Env.GZFileMap.erase(GZFileIt);

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibGZClose_w::body(const Runtime::CallingFrame &,
                                            uint32_t GZFile) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZClose_w] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const auto ZRes = gzclose_w(GZFileIt->second.get());

  Env.GZFileMap.erase(GZFileIt);

  return ZRes;
}

Expect<void> WasmEdgeZlibGZClearerr::body(const Runtime::CallingFrame &,
                                          uint32_t GZFile) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZClearerr] "sv
                  "Invalid GZFile received."sv);
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
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibAdler32] "sv
                  "Frame.getMemoryByIndex(0) returned nullptr."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Buf = MemInst->getPointer<Bytef *>(BufPtr);

  return adler32(Adler, Buf, Len);
}

Expect<int32_t> WasmEdgeZlibAdler32_z::body(const Runtime::CallingFrame &Frame,
                                            uint32_t Adler, uint32_t BufPtr,
                                            uint32_t Len) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibAdler32_z] "sv
                  "Frame.getMemoryByIndex(0) returned nullptr."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Buf = MemInst->getPointer<Bytef *>(BufPtr);

  return adler32_z(Adler, Buf, Len);
}

Expect<int32_t> WasmEdgeZlibAdler32Combine::body(const Runtime::CallingFrame &,
                                                 uint32_t Adler1,
                                                 uint32_t Adler2,
                                                 int32_t Len2) {
  return adler32_combine(Adler1, Adler2, Len2);
}

Expect<int32_t> WasmEdgeZlibCRC32::body(const Runtime::CallingFrame &Frame,
                                        uint32_t CRC, uint32_t BufPtr,
                                        uint32_t Len) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibCRC32] "sv
                  "Frame.getMemoryByIndex(0) returned nullptr."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Buf = MemInst->getPointer<Bytef *>(BufPtr);

  return crc32(CRC, Buf, Len);
}

Expect<int32_t> WasmEdgeZlibCRC32_z::body(const Runtime::CallingFrame &Frame,
                                          uint32_t CRC, uint32_t BufPtr,
                                          uint32_t Len) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibCRC32_z] "sv
                  "Frame.getMemoryByIndex(0) returned nullptr."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *Buf = MemInst->getPointer<Bytef *>(BufPtr);

  return crc32_z(CRC, Buf, Len);
}

Expect<int32_t> WasmEdgeZlibCRC32Combine::body(const Runtime::CallingFrame &,
                                               uint32_t CRC1, uint32_t CRC2,
                                               int32_t Len2) {
  return crc32_combine(CRC1, CRC2, Len2);
}

Expect<int32_t>
WasmEdgeZlibDeflateInit_::body(const Runtime::CallingFrame &Frame,
                               uint32_t ZStreamPtr, int32_t Level,
                               uint32_t VersionPtr, int32_t StreamSize) {
  if (!CheckSize(StreamSize))
    return static_cast<int32_t>(Z_VERSION_ERROR);

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibDeflateInit_] "sv
                  "Frame.getMemoryByIndex(0) returned nullptr."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const auto *WasmZlibVersion = MemInst->getPointer<const char *>(VersionPtr);
  auto HostZStream = std::make_unique<z_stream>();

  // ignore wasm custom allocators
  HostZStream->zalloc = Z_NULL;
  HostZStream->zfree = Z_NULL;
  // ignore opaque since zmalloc and zfree was ignored
  HostZStream->opaque = Z_NULL;

  auto It =
      Env.ZStreamMap.emplace(std::make_pair(ZStreamPtr, std::move(HostZStream)))
          .second;

  const auto ZRes =
      SyncRun("WasmEdgeZlibDeflateInit_", Env, ZStreamPtr, Frame,
              [&](z_stream *HostZStream) {
                return deflateInit_(HostZStream, Level, WasmZlibVersion,
                                    sizeof(z_stream));
              });

  if (ZRes != Z_OK)
    Env.ZStreamMap.erase(It);

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
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibInflateInit_] "sv
                  "Frame.getMemoryByIndex(0) returned nullptr."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const auto *WasmZlibVersion = MemInst->getPointer<const char *>(VersionPtr);
  auto HostZStream = std::make_unique<z_stream>();

  // ignore wasm custom allocators
  HostZStream->zalloc = Z_NULL;
  HostZStream->zfree = Z_NULL;
  // ignore opaque since zmalloc and zfree was ignored
  HostZStream->opaque = Z_NULL;

  auto It =
      Env.ZStreamMap.emplace(std::make_pair(ZStreamPtr, std::move(HostZStream)))
          .second;

  const auto ZRes = SyncRun("WasmEdgeZlibInflateInit_", Env, ZStreamPtr, Frame,
                            [&](z_stream *HostZStream) {
                              return inflateInit_(HostZStream, WasmZlibVersion,
                                                  sizeof(z_stream));
                            });

  if (ZRes != Z_OK)
    Env.ZStreamMap.erase(It);

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
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibDeflateInit2_] "sv
                  "Frame.getMemoryByIndex(0) returned nullptr."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const auto *WasmZlibVersion = MemInst->getPointer<const char *>(VersionPtr);
  auto HostZStream = std::make_unique<z_stream>();
  HostZStream->zalloc = Z_NULL;
  HostZStream->zfree = Z_NULL;
  HostZStream->opaque =
      Z_NULL; // ignore opaque since zmalloc and zfree was ignored

  auto It =
      Env.ZStreamMap.emplace(std::make_pair(ZStreamPtr, std::move(HostZStream)))
          .second;

  const auto ZRes = SyncRun(
      "WasmEdgeZlibDeflateInit2_", Env, ZStreamPtr, Frame,
      [&](z_stream *HostZStream) {
        return deflateInit2_(HostZStream, Level, Method, WindowBits, MemLevel,
                             Strategy, WasmZlibVersion, sizeof(z_stream));
      });

  if (ZRes != Z_OK)
    Env.ZStreamMap.erase(It);

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
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibInflateInit2_] "sv
                  "Frame.getMemoryByIndex(0) returned nullptr."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const auto *WasmZlibVersion = MemInst->getPointer<const char *>(VersionPtr);
  auto HostZStream = std::make_unique<z_stream>();
  HostZStream->zalloc = Z_NULL;
  HostZStream->zfree = Z_NULL;
  HostZStream->opaque =
      Z_NULL; // ignore opaque since zmalloc and zfree was ignored

  auto It =
      Env.ZStreamMap.emplace(std::make_pair(ZStreamPtr, std::move(HostZStream)))
          .second;

  const auto ZRes =
      SyncRun("WasmEdgeZlibInflateInit2_", Env, ZStreamPtr, Frame,
              [&](z_stream *HostZStream) {
                return inflateInit2_(HostZStream, WindowBits, WasmZlibVersion,
                                     sizeof(z_stream));
              });

  if (ZRes != Z_OK)
    Env.ZStreamMap.erase(It);

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibInflateBackInit_::body(
    const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr, int32_t WindowBits,
    uint32_t WindowPtr, uint32_t VersionPtr, int32_t StreamSize) {
  if (!CheckSize(StreamSize))
    return static_cast<int32_t>(Z_VERSION_ERROR);

  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibInflateBackInit_] "sv
                  "Frame.getMemoryByIndex(0) returned nullptr."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const auto *WasmZlibVersion = MemInst->getPointer<const char *>(VersionPtr);
  auto *Window = MemInst->getPointer<unsigned char *>(WindowPtr);
  auto HostZStream = std::make_unique<z_stream>();
  HostZStream->zalloc = Z_NULL;
  HostZStream->zfree = Z_NULL;
  HostZStream->opaque =
      Z_NULL; // ignore opaque since zmalloc and zfree was ignored

  auto It =
      Env.ZStreamMap.emplace(std::make_pair(ZStreamPtr, std::move(HostZStream)))
          .second;

  const auto ZRes =
      SyncRun("WasmEdgeZlibInflateBackInit_", Env, ZStreamPtr, Frame,
              [&](z_stream *HostZStream) {
                return inflateBackInit_(HostZStream, WindowBits, Window,
                                        WasmZlibVersion, sizeof(z_stream));
              });

  if (ZRes != Z_OK)
    Env.ZStreamMap.erase(It);

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibGZGetc_::body(const Runtime::CallingFrame &,
                                          uint32_t GZFile) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZGetc_] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return gzgetc_(GZFileIt->second.get());
}

Expect<int32_t>
WasmEdgeZlibInflateSyncPoint::body(const Runtime::CallingFrame &,
                                   uint32_t ZStreamPtr) {
  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibInflateSyncPoint] "sv
                  "Invalid ZStreamPtr received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return inflateSyncPoint(HostZStreamIt->second.get());
}

Expect<int32_t>
WasmEdgeZlibInflateUndermine::body(const Runtime::CallingFrame &,
                                   uint32_t ZStreamPtr, int32_t Subvert) {
  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibInflateUndermine] "sv
                  "Invalid ZStreamPtr received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return inflateUndermine(HostZStreamIt->second.get(), Subvert);
}

Expect<int32_t> WasmEdgeZlibInflateValidate::body(const Runtime::CallingFrame &,
                                                  uint32_t ZStreamPtr,
                                                  int32_t Check) {
  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibInflateValidate] "sv
                  "Invalid ZStreamPtr received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return inflateValidate(HostZStreamIt->second.get(), Check);
}

Expect<int32_t>
WasmEdgeZlibInflateCodesUsed::body(const Runtime::CallingFrame &,
                                   uint32_t ZStreamPtr) {
  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibInflateCodesUsed] "sv
                  "Invalid ZStreamPtr received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return inflateCodesUsed(HostZStreamIt->second.get());
}

Expect<int32_t>
WasmEdgeZlibInflateResetKeep::body(const Runtime::CallingFrame &,
                                   uint32_t ZStreamPtr) {
  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibInflateResetKeep] "sv
                  "Invalid ZStreamPtr received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return inflateResetKeep(HostZStreamIt->second.get());
}

Expect<int32_t>
WasmEdgeZlibDeflateResetKeep::body(const Runtime::CallingFrame &,
                                   uint32_t ZStreamPtr) {
  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibDeflateResetKeep] "sv
                  "Invalid ZStreamPtr received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return deflateResetKeep(HostZStreamIt->second.get());
}

} // namespace Host
} // namespace WasmEdge
