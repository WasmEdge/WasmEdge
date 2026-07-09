// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "zlibfunc.h"

#include <cstring>

namespace WasmEdge {
namespace Host {

#define MEMINST_CHECK(Out, CallFrame, Index)                                   \
  auto *Out = CallFrame.getMemoryByIndex(Index);                               \
  if (unlikely(Out == nullptr)) {                                              \
    spdlog::error("[WasmEdge-Zlib] Memory instance not found."sv);             \
    return Unexpect(ErrCode::Value::HostFuncError);                            \
  }

// Bind `Var` to a guest buffer of `Len` bytes at `Offset`, validating that the
// whole range is in bounds (zlib may touch all of it). getPointer only checks a
// single element, so getSpan is required here. A zero-length buffer is
// accepted.
#define BUFFER_CHECK(Var, MemInstPtr, Offset, Len, FuncName)                   \
  const auto Var##Span = (MemInstPtr)->getSpan<uint8_t>((Offset), (Len));      \
  if (unlikely((Len) != 0 && Var##Span.data() == nullptr)) {                   \
    spdlog::error("[WasmEdge-Zlib] [" FuncName                                 \
                  "] Out-of-bounds buffer access."sv);                         \
    return Unexpect(ErrCode::Value::HostFuncError);                            \
  }                                                                            \
  auto *Var = Var##Span.data();

// Bind `Var` to a single `Type` object at `Offset`, rejecting an out-of-bounds
// offset (getPointer returns nullptr rather than trapping).
#define PTR_CHECK(Var, MemInstPtr, Offset, Type, FuncName)                     \
  auto *Var = (MemInstPtr)->getPointer<Type *>((Offset));                      \
  if (unlikely(Var == nullptr)) {                                              \
    spdlog::error("[WasmEdge-Zlib] [" FuncName                                 \
                  "] Out-of-bounds pointer access."sv);                        \
    return Unexpect(ErrCode::Value::HostFuncError);                            \
  }

constexpr bool CheckSize(int32_t StreamSize) {

  return (StreamSize == static_cast<int32_t>(sizeof(WasmZStream)));
}

// Return an in-bounds, NUL-terminated C string starting at `Offset`, or nullptr
// when the offset is out of bounds or no terminator exists before the end of
// linear memory (which would make zlib's strlen/strcpy read out of bounds).
static inline const char *
getInBoundsCString(const Runtime::Instance::MemoryInstance &MemInst,
                   uint32_t Offset) noexcept {
  const uint64_t MemSize = MemInst.getSize();
  if (unlikely(Offset >= MemSize)) {
    return nullptr;
  }
  const auto *Str = MemInst.getPointer<const char *>(Offset);
  if (unlikely(Str == nullptr)) {
    return nullptr;
  }
  if (unlikely(std::memchr(Str, '\0', MemSize - Offset) == nullptr)) {
    return nullptr;
  }
  return Str;
}

template <typename T>
auto SyncRun(const std::string_view &Msg, WasmEdgeZlibEnvironment &Env,
             uint32_t ZStreamPtr, const Runtime::CallingFrame &Frame,
             T Callback) -> Expect<int32_t> {

  MEMINST_CHECK(MemInst, Frame, 0)
  WasmZStream *ModuleZStream = MemInst->getPointer<WasmZStream *>(ZStreamPtr);
  if (unlikely(ModuleZStream == nullptr)) {
    spdlog::error("[WasmEdge-Zlib] [{}-SyncRun] "sv
                  "Out-of-bounds ZStreamPtr received."sv,
                  Msg);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const auto HostZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (HostZStreamIt == Env.ZStreamMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [{}-SyncRun] "sv
                  "Invalid ZStreamPtr received."sv,
                  Msg);
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  auto HostZStream = HostZStreamIt->second.get();
  const auto GZHeaderStoreIt = Env.GZHeaderMap.find(ZStreamPtr);

  const auto InSpan = MemInst->getSpan<unsigned char>(ModuleZStream->NextIn,
                                                      ModuleZStream->AvailIn);
  if (unlikely(ModuleZStream->AvailIn != 0 && InSpan.data() == nullptr)) {
    spdlog::error("[WasmEdge-Zlib] [{}-SyncRun] "sv
                  "Out-of-bounds input buffer."sv,
                  Msg);
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  const auto OutSpan = MemInst->getSpan<unsigned char>(ModuleZStream->NextOut,
                                                       ModuleZStream->AvailOut);
  if (unlikely(ModuleZStream->AvailOut != 0 && OutSpan.data() == nullptr)) {
    spdlog::error("[WasmEdge-Zlib] [{}-SyncRun] "sv
                  "Out-of-bounds output buffer."sv,
                  Msg);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  HostZStream->next_in = InSpan.data();
  HostZStream->avail_in = ModuleZStream->AvailIn;
  HostZStream->total_in = ModuleZStream->TotalIn;

  HostZStream->next_out = OutSpan.data();
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
    if (unlikely(ModuleGZHeader == nullptr)) {
      spdlog::error("[WasmEdge-Zlib] [{}-SyncRun] "sv
                    "Out-of-bounds gzip header."sv,
                    Msg);
      return Unexpect(ErrCode::Value::HostFuncError);
    }
    auto *HostGZHeader = GZHeaderStoreIt->second.HostGZHeader.get();
    const bool IsInflate = GZHeaderStoreIt->second.IsInflate;

    HostGZHeader->text = ModuleGZHeader->Text;
    HostGZHeader->time = ModuleGZHeader->Time;
    HostGZHeader->xflags = ModuleGZHeader->XFlags;
    HostGZHeader->os = ModuleGZHeader->OS;

    // inflateGetHeader writes into the buffers (bounded by *_max); the strings
    // are read back as-is. deflateSetHeader reads the buffers: extra spans
    // extra_len bytes and name/comment are zlib-scanned C strings.
    const uint32_t ExtraCap =
        IsInflate ? ModuleGZHeader->ExtraMax : ModuleGZHeader->ExtraLen;
    const auto ExtraSpan =
        MemInst->getSpan<unsigned char>(ModuleGZHeader->Extra, ExtraCap);
    if (unlikely(ExtraCap != 0 && ExtraSpan.data() == nullptr)) {
      spdlog::error("[WasmEdge-Zlib] [{}-SyncRun] "sv
                    "Out-of-bounds gzip header extra field."sv,
                    Msg);
      return Unexpect(ErrCode::Value::HostFuncError);
    }
    HostGZHeader->extra = ExtraSpan.data();
    HostGZHeader->extra_len = ModuleGZHeader->ExtraLen;
    HostGZHeader->extra_max = ModuleGZHeader->ExtraMax;

    const auto BindHeaderString = [&](uint32_t FieldOffset, uint32_t MaxLen,
                                      Bytef *&HostField) -> bool {
      if (IsInflate) {
        const auto FieldSpan =
            MemInst->getSpan<unsigned char>(FieldOffset, MaxLen);
        if (unlikely(MaxLen != 0 && FieldSpan.data() == nullptr)) {
          return false;
        }
        HostField = FieldSpan.data();
      } else if (FieldOffset != 0) {
        const auto *FieldStr = getInBoundsCString(*MemInst, FieldOffset);
        if (unlikely(FieldStr == nullptr)) {
          return false;
        }
        HostField = reinterpret_cast<Bytef *>(const_cast<char *>(FieldStr));
      } else {
        HostField = nullptr;
      }
      return true;
    };

    if (unlikely(
            !BindHeaderString(ModuleGZHeader->Name, ModuleGZHeader->NameMax,
                              HostGZHeader->name) ||
            !BindHeaderString(ModuleGZHeader->Comment, ModuleGZHeader->CommMax,
                              HostGZHeader->comment))) {
      spdlog::error("[WasmEdge-Zlib] [{}-SyncRun] "sv
                    "Out-of-bounds gzip header name/comment field."sv,
                    Msg);
      return Unexpect(ErrCode::Value::HostFuncError);
    }
    HostGZHeader->name_max = ModuleGZHeader->NameMax;
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

  auto NewZStream = std::make_unique<z_stream>();
  NewZStream->zalloc = Z_NULL;
  NewZStream->zfree = Z_NULL;
  NewZStream->opaque =
      Z_NULL; // ignore opaque since zmalloc and zfree was ignored

  const auto [It, Inserted] =
      Env.ZStreamMap.emplace(std::make_pair(ZStreamPtr, std::move(NewZStream)));

  const auto ZRes = SyncRun(
      "WasmEdgeZlibDeflateInit", Env, ZStreamPtr, Frame,
      [&](z_stream *HostZStream) { return deflateInit(HostZStream, Level); });

  if (ZRes != Z_OK && Inserted)
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

  auto NewZStream = std::make_unique<z_stream>();
  NewZStream->zalloc = Z_NULL;
  NewZStream->zfree = Z_NULL;
  NewZStream->opaque =
      Z_NULL; // ignore opaque since zmalloc and zfree was ignored

  const auto [It, Inserted] =
      Env.ZStreamMap.emplace(std::make_pair(ZStreamPtr, std::move(NewZStream)));

  const auto ZRes =
      SyncRun("WasmEdgeZlibInflateInit", Env, ZStreamPtr, Frame,
              [&](z_stream *HostZStream) { return inflateInit(HostZStream); });

  if (ZRes != Z_OK && Inserted)
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

  auto NewZStream = std::make_unique<z_stream>();
  NewZStream->zalloc = Z_NULL;
  NewZStream->zfree = Z_NULL;
  NewZStream->opaque =
      Z_NULL; // ignore opaque since zmalloc and zfree was ignored

  const auto [It, Inserted] =
      Env.ZStreamMap.emplace(std::make_pair(ZStreamPtr, std::move(NewZStream)));

  const auto ZRes =
      SyncRun("WasmEdgeZlibDeflateInit2", Env, ZStreamPtr, Frame,
              [&](z_stream *HostZStream) {
                return deflateInit2(HostZStream, Level, Method, WindowBits,
                                    MemLevel, Strategy);
              });

  if (ZRes != Z_OK && Inserted)
    Env.ZStreamMap.erase(It);

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibDeflateSetDictionary::body(
    const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
    uint32_t DictionaryPtr, uint32_t DictLength) {

  MEMINST_CHECK(MemInst, Frame, 0)

  BUFFER_CHECK(Dictionary, MemInst, DictionaryPtr, DictLength,
               "WasmEdgeZlibDeflateSetDictionary")

  // deflateSetDictionary never reads a zero-length dictionary yet still
  // rejects Z_NULL, so preserve that contract exactly: a guest null maps to
  // Z_NULL while any other offset stays non-null, including one past the end
  // of memory whose zero-length span carries no pointer.
  static constexpr Bytef EmptyDictionary = 0;
  const Bytef *DictionaryArg = Dictionary;
  if (DictLength == 0) {
    DictionaryArg = DictionaryPtr == 0 ? Z_NULL : &EmptyDictionary;
  }

  return SyncRun("WasmEdgeZlibDeflateSetDictionary", Env, ZStreamPtr, Frame,
                 [&](z_stream *HostZStream) {
                   return deflateSetDictionary(HostZStream, DictionaryArg,
                                               DictLength);
                 });
}

Expect<int32_t> WasmEdgeZlibDeflateGetDictionary::body(
    const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
    uint32_t DictionaryPtr, uint32_t DictLengthPtr) {

  MEMINST_CHECK(MemInst, Frame, 0)

  const auto ZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (unlikely(ZStreamIt == Env.ZStreamMap.end())) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibDeflateGetDictionary] "sv
                  "Invalid ZStreamPtr received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  // zlib writes up to the internal window (~32 KiB) with no caller-supplied
  // cap, so query the exact length first and validate the destination for it.
  uInt NeededLen = 0;
  deflateGetDictionary(ZStreamIt->second.get(), Z_NULL, &NeededLen);

  BUFFER_CHECK(Dictionary, MemInst, DictionaryPtr, NeededLen,
               "WasmEdgeZlibDeflateGetDictionary")
  PTR_CHECK(DictLength, MemInst, DictLengthPtr, uint32_t,
            "WasmEdgeZlibDeflateGetDictionary")

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
  auto *SourceZStream = SourceZStreamIt->second.get();

  auto NewZStream = std::make_unique<z_stream>();

  const auto [It, Inserted] =
      Env.ZStreamMap.emplace(std::make_pair(DestPtr, std::move(NewZStream)));

  const auto Res = SyncRun("WasmEdgeZlibDeflateCopy", Env, DestPtr, Frame,
                           [&](z_stream *) { return 0; });
  if (!Res.has_value()) {
    if (Inserted)
      Env.ZStreamMap.erase(It);
    return Res;
  }

  const auto ZRes = SyncRun("WasmEdgeZlibDeflateCopy", Env, DestPtr, Frame,
                            [&](z_stream *DestZStream) {
                              return deflateCopy(DestZStream, SourceZStream);
                            });

  if (ZRes != Z_OK && Inserted)
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

  MEMINST_CHECK(MemInst, Frame, 0)

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

  const auto [It, Inserted] = Env.GZHeaderMap.emplace(
      std::pair<uint32_t, WasmEdgeZlibEnvironment::GZStore>{
          ZStreamPtr, WasmEdgeZlibEnvironment::GZStore{
                          .WasmGZHeaderOffset = HeadPtr,
                          .IsInflate = false,
                          .HostGZHeader = std::move(HostGZHeader)}});

  const auto ZRes =
      SyncRun("WasmEdgeZlibDeflateSetHeader", Env, ZStreamPtr, Frame,
              [&](z_stream *HostZStream) {
                return deflateSetHeader(HostZStream, HostGZHeaderPtr);
              });

  if (ZRes != Z_OK && Inserted)
    Env.GZHeaderMap.erase(It);

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibInflateInit2::body(const Runtime::CallingFrame &Frame,
                               uint32_t ZStreamPtr, int32_t WindowBits) {
  auto NewZStream = std::make_unique<z_stream>();
  NewZStream->zalloc = Z_NULL;
  NewZStream->zfree = Z_NULL;
  NewZStream->opaque =
      Z_NULL; // ignore opaque since zmalloc and zfree was ignored

  const auto [It, Inserted] =
      Env.ZStreamMap.emplace(std::make_pair(ZStreamPtr, std::move(NewZStream)));

  const auto ZRes = SyncRun("WasmEdgeZlibInflateInit2", Env, ZStreamPtr, Frame,
                            [&](z_stream *HostZStream) {
                              return inflateInit2(HostZStream, WindowBits);
                            });

  if (ZRes != Z_OK && Inserted)
    Env.ZStreamMap.erase(It);

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibInflateSetDictionary::body(
    const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
    uint32_t DictionaryPtr, uint32_t DictLength) {

  MEMINST_CHECK(MemInst, Frame, 0)

  BUFFER_CHECK(Dictionary, MemInst, DictionaryPtr, DictLength,
               "WasmEdgeZlibInflateSetDictionary")

  return SyncRun("WasmEdgeZlibInflateSetDictionary", Env, ZStreamPtr, Frame,
                 [&](z_stream *HostZStream) {
                   return inflateSetDictionary(HostZStream, Dictionary,
                                               DictLength);
                 });
}

Expect<int32_t> WasmEdgeZlibInflateGetDictionary::body(
    const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
    uint32_t DictionaryPtr, uint32_t DictLengthPtr) {

  MEMINST_CHECK(MemInst, Frame, 0)

  const auto ZStreamIt = Env.ZStreamMap.find(ZStreamPtr);
  if (unlikely(ZStreamIt == Env.ZStreamMap.end())) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibInflateGetDictionary] "sv
                  "Invalid ZStreamPtr received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  // zlib writes up to the internal window (~32 KiB) with no caller-supplied
  // cap, so query the exact length first and validate the destination for it.
  uInt NeededLen = 0;
  inflateGetDictionary(ZStreamIt->second.get(), Z_NULL, &NeededLen);

  BUFFER_CHECK(Dictionary, MemInst, DictionaryPtr, NeededLen,
               "WasmEdgeZlibInflateGetDictionary")
  PTR_CHECK(DictLength, MemInst, DictLengthPtr, uint32_t,
            "WasmEdgeZlibInflateGetDictionary")

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
  auto *SourceZStream = SourceZStreamIt->second.get();

  auto NewZStream = std::make_unique<z_stream>();

  const auto [It, Inserted] =
      Env.ZStreamMap.emplace(std::make_pair(DestPtr, std::move(NewZStream)));

  const auto Res = SyncRun("WasmEdgeZlibInflateCopy", Env, DestPtr, Frame,
                           [&](z_stream *) { return 0; });
  if (!Res.has_value()) {
    if (Inserted)
      Env.ZStreamMap.erase(It);
    return Res;
  }

  const auto ZRes = SyncRun("WasmEdgeZlibInflateCopy", Env, DestPtr, Frame,
                            [&](z_stream *DestZStream) {
                              return inflateCopy(DestZStream, SourceZStream);
                            });

  if (ZRes != Z_OK && Inserted)
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

  const auto [It, Inserted] = Env.GZHeaderMap.emplace(
      std::pair<uint32_t, WasmEdgeZlibEnvironment::GZStore>{
          ZStreamPtr, WasmEdgeZlibEnvironment::GZStore{
                          .WasmGZHeaderOffset = HeadPtr,
                          .IsInflate = true,
                          .HostGZHeader = std::move(HostGZHeader)}});

  const auto ZRes =
      SyncRun("WasmEdgeZlibInflateGetHeader", Env, ZStreamPtr, Frame,
              [&](z_stream *HostZStream) {
                return inflateGetHeader(HostZStream, HostGZHeaderPtr);
              });

  if (ZRes != Z_OK && Inserted)
    Env.GZHeaderMap.erase(It);

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibInflateBackInit::body(const Runtime::CallingFrame &Frame,
                                  uint32_t ZStreamPtr, int32_t WindowBits,
                                  uint32_t WindowPtr) {
  auto NewZStream = std::make_unique<z_stream>();
  NewZStream->zalloc = Z_NULL;
  NewZStream->zfree = Z_NULL;
  NewZStream->opaque =
      Z_NULL; // ignore opaque since zmalloc and zfree was ignored

  MEMINST_CHECK(MemInst, Frame, 0)

  uint8_t *Window = nullptr;
  if (WindowBits >= 8 && WindowBits <= 15) {
    const auto WindowSpan =
        MemInst->getSpan<uint8_t>(WindowPtr, UINT64_C(1) << WindowBits);
    if (unlikely(WindowSpan.data() == nullptr)) {
      spdlog::error("[WasmEdge-Zlib] [InflateBackInit] "sv
                    "Out-of-bounds window buffer."sv);
      return Unexpect(ErrCode::Value::HostFuncError);
    }
    Window = WindowSpan.data();
  } else {
    Window = MemInst->getPointer<unsigned char *>(WindowPtr);
  }

  const auto [It, Inserted] =
      Env.ZStreamMap.emplace(std::make_pair(ZStreamPtr, std::move(NewZStream)));

  const auto ZRes =
      SyncRun("WasmEdgeZlibInflateBackInit", Env, ZStreamPtr, Frame,
              [&](z_stream *HostZStream) {
                return inflateBackInit(HostZStream, WindowBits, Window);
              });

  if (ZRes != Z_OK && Inserted)
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

// compress/compress2 reject a null destination (Z_STREAM_ERROR) yet answer a
// non-null one that merely lacks capacity with Z_BUF_ERROR, all without
// writing a byte, so mirror deflateSetDictionary's contract: a guest null
// maps to Z_NULL while any other offset stays non-null, including one whose
// zero-length span carries no pointer. uncompress/uncompress2 need no such
// care -- at zero capacity they swap in an internal probe buffer and never
// look at the caller's pointer.
static Bytef ZeroCapacityDest;
static inline Bytef *resolveDestArg(uint8_t *Dest, uint32_t DestPtr,
                                    uint32_t DstCap) noexcept {
  if (DstCap != 0) {
    return Dest;
  }
  return DestPtr == 0 ? Z_NULL : &ZeroCapacityDest;
}

Expect<int32_t> WasmEdgeZlibCompress::body(const Runtime::CallingFrame &Frame,
                                           uint32_t DestPtr,
                                           uint32_t DestLenPtr,
                                           uint32_t SourcePtr,
                                           uint32_t SourceLen) {
  MEMINST_CHECK(MemInst, Frame, 0)

  PTR_CHECK(DestLen, MemInst, DestLenPtr, uint32_t, "WasmEdgeZlibCompress")
  const uint32_t DstCap = *DestLen;
  BUFFER_CHECK(Dest, MemInst, DestPtr, DstCap, "WasmEdgeZlibCompress")
  BUFFER_CHECK(Source, MemInst, SourcePtr, SourceLen, "WasmEdgeZlibCompress")

  unsigned long HostDestLen = DstCap;
  const auto ZRes = compress(resolveDestArg(Dest, DestPtr, DstCap),
                             &HostDestLen, Source, SourceLen);
  *DestLen = static_cast<uint32_t>(HostDestLen);

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibCompress2::body(const Runtime::CallingFrame &Frame,
                                            uint32_t DestPtr,
                                            uint32_t DestLenPtr,
                                            uint32_t SourcePtr,
                                            uint32_t SourceLen, int32_t Level) {
  MEMINST_CHECK(MemInst, Frame, 0)

  PTR_CHECK(DestLen, MemInst, DestLenPtr, uint32_t, "WasmEdgeZlibCompress2")
  // An out-of-range level fails compress2's deflateInit before any source
  // byte is read or dest byte written -- though only after *destLen has been
  // zeroed -- so answer the same way instead of validating spans the
  // rejected call would never touch.
  if (Level != Z_DEFAULT_COMPRESSION && (Level < 0 || Level > 9)) {
    *DestLen = 0;
    return Z_STREAM_ERROR;
  }
  const uint32_t DstCap = *DestLen;
  BUFFER_CHECK(Dest, MemInst, DestPtr, DstCap, "WasmEdgeZlibCompress2")
  BUFFER_CHECK(Source, MemInst, SourcePtr, SourceLen, "WasmEdgeZlibCompress2")

  unsigned long HostDestLen = DstCap;
  const auto ZRes = compress2(resolveDestArg(Dest, DestPtr, DstCap),
                              &HostDestLen, Source, SourceLen, Level);
  *DestLen = static_cast<uint32_t>(HostDestLen);

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
  MEMINST_CHECK(MemInst, Frame, 0)

  PTR_CHECK(DestLen, MemInst, DestLenPtr, uint32_t, "WasmEdgeZlibUncompress")
  const uint32_t DstCap = *DestLen;
  BUFFER_CHECK(Dest, MemInst, DestPtr, DstCap, "WasmEdgeZlibUncompress")
  BUFFER_CHECK(Source, MemInst, SourcePtr, SourceLen, "WasmEdgeZlibUncompress")

  unsigned long HostDestLen = DstCap;
  const auto ZRes = uncompress(Dest, &HostDestLen, Source, SourceLen);
  *DestLen = static_cast<uint32_t>(HostDestLen);

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibUncompress2::body(const Runtime::CallingFrame &Frame,
                              uint32_t DestPtr, uint32_t DestLenPtr,
                              uint32_t SourcePtr, uint32_t SourceLenPtr) {
  MEMINST_CHECK(MemInst, Frame, 0)

  PTR_CHECK(DestLen, MemInst, DestLenPtr, uint32_t, "WasmEdgeZlibUncompress2")
  PTR_CHECK(SourceLen, MemInst, SourceLenPtr, uint32_t,
            "WasmEdgeZlibUncompress2")
  const uint32_t DstCap = *DestLen;
  const uint32_t SrcLen = *SourceLen;
  BUFFER_CHECK(Dest, MemInst, DestPtr, DstCap, "WasmEdgeZlibUncompress2")
  BUFFER_CHECK(Source, MemInst, SourcePtr, SrcLen, "WasmEdgeZlibUncompress2")

  unsigned long HostDestLen = DstCap, HostSourceLen = SrcLen;
  const auto ZRes = uncompress2(Dest, &HostDestLen, Source, &HostSourceLen);
  // Write the source length before the destination length so an aliased guest
  // (DestLenPtr == SourceLenPtr) is left with the produced length, matching
  // native uncompress2, which writes *destLen last into the shared slot.
  *SourceLen = static_cast<uint32_t>(HostSourceLen);
  *DestLen = static_cast<uint32_t>(HostDestLen);

  return ZRes;
}

Expect<uint32_t> WasmEdgeZlibGZOpen::body(const Runtime::CallingFrame &Frame,
                                          uint32_t PathPtr, uint32_t ModePtr) {
  MEMINST_CHECK(MemInst, Frame, 0)

  const auto *Path = getInBoundsCString(*MemInst, PathPtr);
  const auto *Mode = getInBoundsCString(*MemInst, ModePtr);
  if (unlikely(Path == nullptr || Mode == nullptr)) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZOpen] "sv
                  "Out-of-bounds path or mode string."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto ZRes = gzopen(Path, Mode);
  if (unlikely(ZRes == nullptr)) {
    // A null gzFile is zlib's ordinary open failure (e.g. a missing file);
    // surface it to the guest as a null handle instead of trapping. No live
    // handle is ever 0 because NextGZFile starts at sizeof(gzFile).
    return UINT32_C(0);
  }

  const uint32_t NewWasmGZFile = Env.NextGZFile++;
  Env.GZFileMap.emplace(NewWasmGZFile, ZRes);

  return NewWasmGZFile;
}

Expect<uint32_t> WasmEdgeZlibGZDOpen::body(const Runtime::CallingFrame &Frame,
                                           int32_t FD, uint32_t ModePtr) {
  MEMINST_CHECK(MemInst, Frame, 0)

  const auto *Mode = getInBoundsCString(*MemInst, ModePtr);
  if (unlikely(Mode == nullptr)) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZDOpen] "sv
                  "Out-of-bounds mode string."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto ZRes = gzdopen(FD, Mode);
  if (unlikely(ZRes == nullptr)) {
    // Match gzopen: a null gzFile is an ordinary failure handed back to the
    // guest as a null handle, not a host trap.
    return UINT32_C(0);
  }

  const uint32_t NewWasmGZFile = Env.NextGZFile++;
  Env.GZFileMap.emplace(NewWasmGZFile, ZRes);

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

  return gzbuffer(GZFileIt->second, Size);
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

  return gzsetparams(GZFileIt->second, Level, Strategy);
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

  MEMINST_CHECK(MemInst, Frame, 0)

  BUFFER_CHECK(Buf, MemInst, BufPtr, Len, "WasmEdgeZlibGZRead")

  return gzread(GZFileIt->second, Buf, Len);
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

  MEMINST_CHECK(MemInst, Frame, 0)

  const uint64_t Bytes = static_cast<uint64_t>(Size) * NItems;
  BUFFER_CHECK(Buf, MemInst, BufPtr, Bytes, "WasmEdgeZlibGZFread")

  return gzfread(Buf, Size, NItems, GZFileIt->second);
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

  MEMINST_CHECK(MemInst, Frame, 0)

  BUFFER_CHECK(Buf, MemInst, BufPtr, Len, "WasmEdgeZlibGZWrite")

  return gzwrite(GZFileIt->second, Buf, Len);
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

  MEMINST_CHECK(MemInst, Frame, 0)

  const uint64_t Bytes = static_cast<uint64_t>(Size) * NItems;
  BUFFER_CHECK(Buf, MemInst, BufPtr, Bytes, "WasmEdgeZlibGZFwrite")

  return gzfwrite(Buf, Size, NItems, GZFileIt->second);
}

Expect<int32_t> WasmEdgeZlibGZPuts::body(const Runtime::CallingFrame &Frame,
                                         uint32_t GZFile, uint32_t StringPtr) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZPuts] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  MEMINST_CHECK(MemInst, Frame, 0)

  const auto *String = getInBoundsCString(*MemInst, StringPtr);
  if (unlikely(String == nullptr)) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZPuts] "sv
                  "Out-of-bounds string."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return gzputs(GZFileIt->second, String);
}

Expect<int32_t> WasmEdgeZlibGZPutc::body(const Runtime::CallingFrame &,
                                         uint32_t GZFile, int32_t C) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZPutc] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return gzputc(GZFileIt->second, C);
}

Expect<int32_t> WasmEdgeZlibGZGetc::body(const Runtime::CallingFrame &,
                                         uint32_t GZFile) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZGetc] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return gzgetc(GZFileIt->second);
}

Expect<int32_t> WasmEdgeZlibGZUngetc::body(const Runtime::CallingFrame &,
                                           int32_t C, uint32_t GZFile) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZUngetc] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return gzungetc(C, GZFileIt->second);
}

Expect<int32_t> WasmEdgeZlibGZFlush::body(const Runtime::CallingFrame &,
                                          uint32_t GZFile, int32_t Flush) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZFlush] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return gzflush(GZFileIt->second, Flush);
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

  return gzseek(GZFileIt->second, Offset, Whence);
}

Expect<int32_t> WasmEdgeZlibGZRewind::body(const Runtime::CallingFrame &,
                                           uint32_t GZFile) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZRewind] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return gzrewind(GZFileIt->second);
}

Expect<int32_t> WasmEdgeZlibGZTell::body(const Runtime::CallingFrame &,
                                         uint32_t GZFile) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZTell] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return gztell(GZFileIt->second);
}

Expect<int32_t> WasmEdgeZlibGZOffset::body(const Runtime::CallingFrame &,
                                           uint32_t GZFile) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZOffset] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return gzoffset(GZFileIt->second);
}

Expect<int32_t> WasmEdgeZlibGZEof::body(const Runtime::CallingFrame &,
                                        uint32_t GZFile) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZEof] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return gzeof(GZFileIt->second);
}

Expect<int32_t> WasmEdgeZlibGZDirect::body(const Runtime::CallingFrame &,
                                           uint32_t GZFile) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZDirect] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return gzdirect(GZFileIt->second);
}

Expect<int32_t> WasmEdgeZlibGZClose::body(const Runtime::CallingFrame &,
                                          uint32_t GZFile) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZClose] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const auto ZRes = gzclose(GZFileIt->second);

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

  const auto ZRes = gzclose_r(GZFileIt->second);

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

  const auto ZRes = gzclose_w(GZFileIt->second);

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

  gzclearerr(GZFileIt->second);

  return Expect<void>{};
}

// adler32/crc32 read Z_NULL as a request for the checksum's initial value and
// any non-null zero-length buffer as "leave the running value unchanged",
// mirroring deflateSetDictionary's contract above. getSpan yields a null
// pointer for an out-of-bounds zero-length slice (which would reset the
// caller's checksum) and a non-null pointer for guest offset 0 -- the guest's
// Z_NULL -- (which would suppress the reset it asked for). Map a guest null to
// Z_NULL and every other zero-length request to a non-null empty buffer.
static inline const Bytef *
normalizeChecksumBuf(const Bytef *Buf, uint32_t BufPtr, uint32_t Len) noexcept {
  if (Len != 0) {
    return Buf;
  }
  static constexpr Bytef EmptyBuffer = 0;
  return BufPtr == 0 ? Z_NULL : &EmptyBuffer;
}

Expect<int32_t> WasmEdgeZlibAdler32::body(const Runtime::CallingFrame &Frame,
                                          uint32_t Adler, uint32_t BufPtr,
                                          uint32_t Len) {
  MEMINST_CHECK(MemInst, Frame, 0)

  BUFFER_CHECK(Buf, MemInst, BufPtr, Len, "WasmEdgeZlibAdler32")

  return adler32(Adler, normalizeChecksumBuf(Buf, BufPtr, Len), Len);
}

Expect<int32_t> WasmEdgeZlibAdler32_z::body(const Runtime::CallingFrame &Frame,
                                            uint32_t Adler, uint32_t BufPtr,
                                            uint32_t Len) {
  MEMINST_CHECK(MemInst, Frame, 0)

  BUFFER_CHECK(Buf, MemInst, BufPtr, Len, "WasmEdgeZlibAdler32_z")

  return adler32_z(Adler, normalizeChecksumBuf(Buf, BufPtr, Len), Len);
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
  MEMINST_CHECK(MemInst, Frame, 0)

  BUFFER_CHECK(Buf, MemInst, BufPtr, Len, "WasmEdgeZlibCRC32")

  return crc32(CRC, normalizeChecksumBuf(Buf, BufPtr, Len), Len);
}

Expect<int32_t> WasmEdgeZlibCRC32_z::body(const Runtime::CallingFrame &Frame,
                                          uint32_t CRC, uint32_t BufPtr,
                                          uint32_t Len) {
  MEMINST_CHECK(MemInst, Frame, 0)

  BUFFER_CHECK(Buf, MemInst, BufPtr, Len, "WasmEdgeZlibCRC32_z")

  return crc32_z(CRC, normalizeChecksumBuf(Buf, BufPtr, Len), Len);
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

  MEMINST_CHECK(MemInst, Frame, 0)

  const auto *WasmZlibVersion = MemInst->getPointer<const char *>(VersionPtr);
  auto NewZStream = std::make_unique<z_stream>();

  // ignore wasm custom allocators
  NewZStream->zalloc = Z_NULL;
  NewZStream->zfree = Z_NULL;
  // Ignore opaque because zmalloc and zfree are ignored.
  NewZStream->opaque = Z_NULL;

  const auto [It, Inserted] =
      Env.ZStreamMap.emplace(std::make_pair(ZStreamPtr, std::move(NewZStream)));

  const auto ZRes =
      SyncRun("WasmEdgeZlibDeflateInit_", Env, ZStreamPtr, Frame,
              [&](z_stream *HostZStream) {
                return deflateInit_(HostZStream, Level, WasmZlibVersion,
                                    sizeof(z_stream));
              });

  if (ZRes != Z_OK && Inserted)
    Env.ZStreamMap.erase(It);

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibInflateInit_::body(const Runtime::CallingFrame &Frame,
                               uint32_t ZStreamPtr, uint32_t VersionPtr,
                               int32_t StreamSize) {
  if (!CheckSize(StreamSize))
    return static_cast<int32_t>(Z_VERSION_ERROR);

  MEMINST_CHECK(MemInst, Frame, 0)

  const auto *WasmZlibVersion = MemInst->getPointer<const char *>(VersionPtr);
  auto NewZStream = std::make_unique<z_stream>();

  // ignore wasm custom allocators
  NewZStream->zalloc = Z_NULL;
  NewZStream->zfree = Z_NULL;
  // Ignore opaque because zmalloc and zfree are ignored.
  NewZStream->opaque = Z_NULL;

  const auto [It, Inserted] =
      Env.ZStreamMap.emplace(std::make_pair(ZStreamPtr, std::move(NewZStream)));

  const auto ZRes = SyncRun("WasmEdgeZlibInflateInit_", Env, ZStreamPtr, Frame,
                            [&](z_stream *HostZStream) {
                              return inflateInit_(HostZStream, WasmZlibVersion,
                                                  sizeof(z_stream));
                            });

  if (ZRes != Z_OK && Inserted)
    Env.ZStreamMap.erase(It);

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibDeflateInit2_::body(
    const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr, int32_t Level,
    int32_t Method, int32_t WindowBits, int32_t MemLevel, int32_t Strategy,
    uint32_t VersionPtr, int32_t StreamSize) {
  if (!CheckSize(StreamSize))
    return static_cast<int32_t>(Z_VERSION_ERROR);

  MEMINST_CHECK(MemInst, Frame, 0)

  const auto *WasmZlibVersion = MemInst->getPointer<const char *>(VersionPtr);
  auto NewZStream = std::make_unique<z_stream>();
  NewZStream->zalloc = Z_NULL;
  NewZStream->zfree = Z_NULL;
  NewZStream->opaque =
      Z_NULL; // ignore opaque since zmalloc and zfree was ignored

  const auto [It, Inserted] =
      Env.ZStreamMap.emplace(std::make_pair(ZStreamPtr, std::move(NewZStream)));

  const auto ZRes = SyncRun(
      "WasmEdgeZlibDeflateInit2_", Env, ZStreamPtr, Frame,
      [&](z_stream *HostZStream) {
        return deflateInit2_(HostZStream, Level, Method, WindowBits, MemLevel,
                             Strategy, WasmZlibVersion, sizeof(z_stream));
      });

  if (ZRes != Z_OK && Inserted)
    Env.ZStreamMap.erase(It);

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibInflateInit2_::body(const Runtime::CallingFrame &Frame,
                                uint32_t ZStreamPtr, int32_t WindowBits,
                                uint32_t VersionPtr, int32_t StreamSize) {
  if (!CheckSize(StreamSize))
    return static_cast<int32_t>(Z_VERSION_ERROR);

  MEMINST_CHECK(MemInst, Frame, 0)

  const auto *WasmZlibVersion = MemInst->getPointer<const char *>(VersionPtr);
  auto NewZStream = std::make_unique<z_stream>();
  NewZStream->zalloc = Z_NULL;
  NewZStream->zfree = Z_NULL;
  NewZStream->opaque =
      Z_NULL; // ignore opaque since zmalloc and zfree was ignored

  const auto [It, Inserted] =
      Env.ZStreamMap.emplace(std::make_pair(ZStreamPtr, std::move(NewZStream)));

  const auto ZRes =
      SyncRun("WasmEdgeZlibInflateInit2_", Env, ZStreamPtr, Frame,
              [&](z_stream *HostZStream) {
                return inflateInit2_(HostZStream, WindowBits, WasmZlibVersion,
                                     sizeof(z_stream));
              });

  if (ZRes != Z_OK && Inserted)
    Env.ZStreamMap.erase(It);

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibInflateBackInit_::body(
    const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr, int32_t WindowBits,
    uint32_t WindowPtr, uint32_t VersionPtr, int32_t StreamSize) {
  if (!CheckSize(StreamSize))
    return static_cast<int32_t>(Z_VERSION_ERROR);

  MEMINST_CHECK(MemInst, Frame, 0)

  const auto *WasmZlibVersion = MemInst->getPointer<const char *>(VersionPtr);
  uint8_t *Window = nullptr;
  if (WindowBits >= 8 && WindowBits <= 15) {
    const auto WindowSpan =
        MemInst->getSpan<uint8_t>(WindowPtr, UINT64_C(1) << WindowBits);
    if (unlikely(WindowSpan.data() == nullptr)) {
      spdlog::error("[WasmEdge-Zlib] [InflateBackInit] "sv
                    "Out-of-bounds window buffer."sv);
      return Unexpect(ErrCode::Value::HostFuncError);
    }
    Window = WindowSpan.data();
  } else {
    Window = MemInst->getPointer<unsigned char *>(WindowPtr);
  }
  auto NewZStream = std::make_unique<z_stream>();
  NewZStream->zalloc = Z_NULL;
  NewZStream->zfree = Z_NULL;
  NewZStream->opaque =
      Z_NULL; // ignore opaque since zmalloc and zfree was ignored

  const auto [It, Inserted] =
      Env.ZStreamMap.emplace(std::make_pair(ZStreamPtr, std::move(NewZStream)));

  const auto ZRes =
      SyncRun("WasmEdgeZlibInflateBackInit_", Env, ZStreamPtr, Frame,
              [&](z_stream *HostZStream) {
                return inflateBackInit_(HostZStream, WindowBits, Window,
                                        WasmZlibVersion, sizeof(z_stream));
              });

  if (ZRes != Z_OK && Inserted)
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

  return gzgetc_(GZFileIt->second);
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
