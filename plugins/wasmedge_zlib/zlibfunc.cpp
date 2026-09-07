// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "zlibfunc.h"

#include "host/wasi/wasimodule.h"

#include <cstring>
#include <optional>
#include <string_view>

#if !defined(_WIN32)
#include <fcntl.h>
#include <unistd.h>
#endif

namespace WasmEdge {
namespace Host {

#define MEMINST_CHECK(Out, CallFrame, Index)                                   \
  auto *Out = CallFrame.getMemoryByIndex(Index);                               \
  if (unlikely(Out == nullptr)) {                                              \
    spdlog::error("[WasmEdge-Zlib] Memory instance not found."sv);             \
    return Unexpect(ErrCode::Value::HostFuncError);                            \
  }

#define BUFFER_CHECK(Var, MemInstPtr, Offset, Len, FuncName)                   \
  const auto Var##Span = (MemInstPtr)->getSpan<uint8_t>((Offset), (Len));      \
  if (unlikely((Len) != 0 && Var##Span.data() == nullptr)) {                   \
    spdlog::error("[WasmEdge-Zlib] [" FuncName                                 \
                  "] Out-of-bounds buffer access."sv);                         \
    return Unexpect(ErrCode::Value::HostFuncError);                            \
  }                                                                            \
  auto *Var = Var##Span.data();

#define PTR_CHECK(Var, MemInstPtr, Offset, Type, FuncName)                     \
  auto *Var = (MemInstPtr)->getPointer<Type *>((Offset));                      \
  if (unlikely(Var == nullptr)) {                                              \
    spdlog::error("[WasmEdge-Zlib] [" FuncName                                 \
                  "] Out-of-bounds pointer access."sv);                        \
    return Unexpect(ErrCode::Value::HostFuncError);                            \
  }

static inline bool CheckVersion(const char *Version) noexcept {
  return Version != nullptr && Version[0] == ZLIB_VERSION[0];
}

constexpr bool CheckSize(int32_t StreamSize) {

  return (StreamSize == static_cast<int32_t>(sizeof(WasmZStream)));
}

constexpr uint64_t MaxGZHeaderStringLen = UINT64_C(1) << 20;

constexpr uint64_t MaxZlibVersionLen = 63;

static inline std::optional<std::string_view>
getBoundedInBoundsCString(const Runtime::Instance::MemoryInstance &MemInst,
                          uint32_t Offset, uint64_t MaxLen) noexcept {
  const uint64_t MemSize = MemInst.getSize();
  if (unlikely(Offset >= MemSize)) {
    return std::nullopt;
  }
  const auto *Str = MemInst.getPointer<const char *>(Offset);
  if (unlikely(Str == nullptr)) {
    return std::nullopt;
  }
  const uint64_t Remaining = MemSize - Offset;
  const uint64_t Bound = MaxLen == UINT64_MAX ? Remaining : MaxLen + 1;
  const uint64_t ScanLen = Remaining < Bound ? Remaining : Bound;
  const auto *End = static_cast<const char *>(std::memchr(Str, '\0', ScanLen));
  if (unlikely(End == nullptr)) {
    return std::nullopt;
  }
  return std::string_view(Str, static_cast<size_t>(End - Str));
}

template <typename T>
auto SyncRun(const std::string_view &Msg, WasmEdgeZlibEnvironment &Env,
             uint32_t ZStreamPtr, const Runtime::CallingFrame &Frame,
             T Callback, bool ValidateInputBuffer = false,
             bool ValidateOutputBuffer = false, bool SyncGZHeader = false)
    -> Expect<int32_t> {

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
  auto *HostZStream = &HostZStreamIt->second.Z;
  const auto GZHeaderStoreIt = Env.GZHeaderMap.find(ZStreamPtr);

  const auto InSpan = MemInst->getSpan<unsigned char>(ModuleZStream->NextIn,
                                                      ModuleZStream->AvailIn);
  const auto OutSpan = MemInst->getSpan<unsigned char>(ModuleZStream->NextOut,
                                                       ModuleZStream->AvailOut);
  if (ValidateInputBuffer) {
    if (unlikely(ModuleZStream->AvailIn != 0 && InSpan.data() == nullptr)) {
      spdlog::error("[WasmEdge-Zlib] [{}-SyncRun] "sv
                    "Out-of-bounds input buffer."sv,
                    Msg);
      return Unexpect(ErrCode::Value::HostFuncError);
    }
  }
  if (ValidateOutputBuffer) {
    if (unlikely(ModuleZStream->AvailOut != 0 && OutSpan.data() == nullptr)) {
      spdlog::error("[WasmEdge-Zlib] [{}-SyncRun] "sv
                    "Out-of-bounds output buffer."sv,
                    Msg);
      return Unexpect(ErrCode::Value::HostFuncError);
    }
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

  if (SyncGZHeader && GZHeaderStoreIt != Env.GZHeaderMap.end() &&
      GZHeaderStoreIt->second->IsInflate) {
    auto *ModuleGZHeader = MemInst->getPointer<WasmGZHeader *>(
        GZHeaderStoreIt->second->WasmGZHeaderOffset);
    if (unlikely(ModuleGZHeader == nullptr)) {
      spdlog::error("[WasmEdge-Zlib] [{}-SyncRun] "sv
                    "Out-of-bounds gzip header."sv,
                    Msg);
      return Unexpect(ErrCode::Value::HostFuncError);
    }
    auto *HostGZHeader = GZHeaderStoreIt->second->HostGZHeader.get();

    bool FieldOOB = false;
    static unsigned char ZeroCapacityHeaderField = 0;
    const auto ResolveField = [&](uint32_t Ptr,
                                  uint32_t Max) -> unsigned char * {
      if (Ptr == 0) {
        return nullptr;
      }
      const auto FieldSpan = MemInst->getSpan<unsigned char>(Ptr, Max);
      if (FieldSpan.data() == nullptr) {
        if (unlikely(Max != 0)) {
          FieldOOB = true;
        } else {
          return &ZeroCapacityHeaderField;
        }
      }
      return FieldSpan.data();
    };
    HostGZHeader->extra =
        ResolveField(ModuleGZHeader->Extra, ModuleGZHeader->ExtraMax);
    HostGZHeader->extra_max = ModuleGZHeader->ExtraMax;
    HostGZHeader->name =
        ResolveField(ModuleGZHeader->Name, ModuleGZHeader->NameMax);
    HostGZHeader->name_max = ModuleGZHeader->NameMax;
    HostGZHeader->comment =
        ResolveField(ModuleGZHeader->Comment, ModuleGZHeader->CommMax);
    HostGZHeader->comm_max = ModuleGZHeader->CommMax;
    if (unlikely(FieldOOB)) {
      spdlog::error("[WasmEdge-Zlib] [{}-SyncRun] "sv
                    "Out-of-bounds gzip header buffer."sv,
                    Msg);
      return Unexpect(ErrCode::Value::HostFuncError);
    }

    PreComputeExtra = HostGZHeader->extra;
    PreComputeName = HostGZHeader->name;
    PreComputeComment = HostGZHeader->comment;
  }

  const auto ZRes = Callback(HostZStream);

  if (HostZStream->next_in != nullptr && PreComputeNextIn != nullptr) {
    ModuleZStream->NextIn += HostZStream->next_in - PreComputeNextIn;
  }
  ModuleZStream->AvailIn = HostZStream->avail_in;
  ModuleZStream->TotalIn = HostZStream->total_in;

  if (HostZStream->next_out != nullptr && PreComputeNextOut != nullptr) {
    ModuleZStream->NextOut += HostZStream->next_out - PreComputeNextOut;
  }
  ModuleZStream->AvailOut = HostZStream->avail_out;
  ModuleZStream->TotalOut = HostZStream->total_out;

  // TODO: ignore msg for now
  // ignore state
  // ignore zalloc, zfree, opaque

  ModuleZStream->DataType = HostZStream->data_type;
  ModuleZStream->Adler = HostZStream->adler;
  ModuleZStream->Reserved = HostZStream->reserved;

  if (SyncGZHeader && GZHeaderStoreIt != Env.GZHeaderMap.end() &&
      GZHeaderStoreIt->second->IsInflate) {
    auto *ModuleGZHeader = MemInst->getPointer<WasmGZHeader *>(
        GZHeaderStoreIt->second->WasmGZHeaderOffset);
    if (ModuleGZHeader != nullptr) {
      auto *HostGZHeader = GZHeaderStoreIt->second->HostGZHeader.get();

      ModuleGZHeader->Text = HostGZHeader->text;
      ModuleGZHeader->Time = HostGZHeader->time;
      ModuleGZHeader->XFlags = HostGZHeader->xflags;
      ModuleGZHeader->OS = HostGZHeader->os;

      if (HostGZHeader->extra == Z_NULL) {
        ModuleGZHeader->Extra = 0;
      } else if (PreComputeExtra != nullptr) {
        ModuleGZHeader->Extra += HostGZHeader->extra - PreComputeExtra;
      }
      ModuleGZHeader->ExtraLen = HostGZHeader->extra_len;
      ModuleGZHeader->ExtraMax = HostGZHeader->extra_max;

      if (HostGZHeader->name == Z_NULL) {
        ModuleGZHeader->Name = 0;
      } else if (PreComputeName != nullptr) {
        ModuleGZHeader->Name += HostGZHeader->name - PreComputeName;
      }
      ModuleGZHeader->NameMax = HostGZHeader->name_max;

      if (HostGZHeader->comment == Z_NULL) {
        ModuleGZHeader->Comment = 0;
      } else if (PreComputeComment != nullptr) {
        ModuleGZHeader->Comment += HostGZHeader->comment - PreComputeComment;
      }
      ModuleGZHeader->CommMax = HostGZHeader->comm_max;

      ModuleGZHeader->HCRC = HostGZHeader->hcrc;
      ModuleGZHeader->Done = HostGZHeader->done;
    }
  }

  return ZRes;
}

template <typename InitFn>
Expect<int32_t> initStream(const std::string_view &Msg,
                           WasmEdgeZlibEnvironment &Env, uint32_t ZStreamPtr,
                           const Runtime::CallingFrame &Frame, ZStreamKind Kind,
                           InitFn Init) {
  const auto [It, Inserted] = Env.ZStreamMap.try_emplace(ZStreamPtr, Kind);
  if (unlikely(!Inserted)) {
    spdlog::error("[WasmEdge-Zlib] [{}] "
                  "Re-initializing a stream that is still live."sv,
                  Msg);
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }

  const auto ZRes = SyncRun(Msg, Env, ZStreamPtr, Frame, Init);

  if (!ZRes || *ZRes != Z_OK)
    Env.ZStreamMap.erase(It);

  return ZRes;
}

static inline bool streamKindMismatch(const WasmEdgeZlibEnvironment &Env,
                                      uint32_t ZStreamPtr,
                                      ZStreamKind Expected) noexcept {
  const auto It = Env.ZStreamMap.find(ZStreamPtr);
  return It != Env.ZStreamMap.end() && It->second.Kind != Expected;
}

Expect<int32_t>
WasmEdgeZlibDeflateInit::body(const Runtime::CallingFrame &Frame,
                              uint32_t ZStreamPtr, int32_t Level) {
  return initStream(
      "WasmEdgeZlibDeflateInit", Env, ZStreamPtr, Frame, ZStreamKind::Deflate,
      [&](z_stream *HostZStream) { return deflateInit(HostZStream, Level); });
}

Expect<int32_t> WasmEdgeZlibDeflate::WasmEdgeZlibDeflate::body(
    const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr, int32_t Flush) {

  if (streamKindMismatch(Env, ZStreamPtr, ZStreamKind::Deflate)) {
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }

  const bool FlushValid = Flush >= Z_NO_FLUSH && Flush <= Z_BLOCK;
  const auto ZRes = SyncRun(
      "WasmEdgeZlibDeflate", Env, ZStreamPtr, Frame,
      [&](z_stream *HostZStream) { return deflate(HostZStream, Flush); },
      /*ValidateInputBuffer=*/FlushValid, /*ValidateOutputBuffer=*/FlushValid);

  if (ZRes && (*ZRes == Z_OK || *ZRes == Z_STREAM_END)) {
    if (const auto It = Env.ZStreamMap.find(ZStreamPtr);
        It != Env.ZStreamMap.end()) {
      It->second.DeflateStarted = true;
    }
  }

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibDeflateEnd::body(const Runtime::CallingFrame &Frame,
                                             uint32_t ZStreamPtr) {

  if (streamKindMismatch(Env, ZStreamPtr, ZStreamKind::Deflate)) {
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }

  const auto ZRes =
      SyncRun("WasmEdgeZlibDeflateEnd", Env, ZStreamPtr, Frame,
              [&](z_stream *HostZStream) { return deflateEnd(HostZStream); });

  if (ZRes && (*ZRes == Z_OK || *ZRes == Z_DATA_ERROR)) {
    Env.ZStreamMap.erase(ZStreamPtr);
    Env.GZHeaderMap.erase(ZStreamPtr);
  }

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibInflateInit::body(const Runtime::CallingFrame &Frame,
                              uint32_t ZStreamPtr) {
  return initStream(
      "WasmEdgeZlibInflateInit", Env, ZStreamPtr, Frame, ZStreamKind::Inflate,
      [&](z_stream *HostZStream) { return inflateInit(HostZStream); });
}

Expect<int32_t> WasmEdgeZlibInflate::body(const Runtime::CallingFrame &Frame,
                                          uint32_t ZStreamPtr, int32_t Flush) {

  if (streamKindMismatch(Env, ZStreamPtr, ZStreamKind::Inflate)) {
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }

  const auto ZRes = SyncRun(
      "WasmEdgeZlibInflate", Env, ZStreamPtr, Frame,
      [&](z_stream *HostZStream) { return inflate(HostZStream, Flush); },
      /*ValidateInputBuffer=*/true, /*ValidateOutputBuffer=*/true,
      /*SyncGZHeader=*/true);

  if (ZRes) {
    if (const auto It = Env.ZStreamMap.find(ZStreamPtr);
        It != Env.ZStreamMap.end()) {
      It->second.MayNeedDict = *ZRes == Z_NEED_DICT;
    }
  }

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibInflateEnd::body(const Runtime::CallingFrame &Frame,
                                             uint32_t ZStreamPtr) {

  if (streamKindMismatch(Env, ZStreamPtr, ZStreamKind::Inflate)) {
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }

  const auto ZRes =
      SyncRun("WasmEdgeZlibInflateEnd", Env, ZStreamPtr, Frame,
              [&](z_stream *HostZStream) { return inflateEnd(HostZStream); });

  if (ZRes && *ZRes != Z_STREAM_ERROR) {
    Env.ZStreamMap.erase(ZStreamPtr);
    Env.GZHeaderMap.erase(ZStreamPtr);
  }

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibDeflateInit2::body(
    const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr, int32_t Level,
    int32_t Method, int32_t WindowBits, int32_t MemLevel, int32_t Strategy) {
  const auto ZRes =
      initStream("WasmEdgeZlibDeflateInit2", Env, ZStreamPtr, Frame,
                 ZStreamKind::Deflate, [&](z_stream *HostZStream) {
                   return deflateInit2(HostZStream, Level, Method, WindowBits,
                                       MemLevel, Strategy);
                 });
  if (ZRes && *ZRes == Z_OK) {
    if (const auto It = Env.ZStreamMap.find(ZStreamPtr);
        It != Env.ZStreamMap.end()) {
      It->second.GzipWrap = WindowBits >= 16;
      It->second.RawDeflate = WindowBits < 0;
    }
  }
  return ZRes;
}

Expect<int32_t> WasmEdgeZlibDeflateSetDictionary::body(
    const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
    uint32_t DictionaryPtr, uint32_t DictLength) {

  if (const auto It = Env.ZStreamMap.find(ZStreamPtr);
      It != Env.ZStreamMap.end() &&
      (It->second.Kind != ZStreamKind::Deflate || It->second.GzipWrap ||
       (!It->second.RawDeflate && It->second.DeflateStarted))) {
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }

  MEMINST_CHECK(MemInst, Frame, 0)

  BUFFER_CHECK(Dictionary, MemInst, DictionaryPtr, DictLength,
               "WasmEdgeZlibDeflateSetDictionary")

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
  if (ZStreamIt->second.Kind != ZStreamKind::Deflate) {
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }

  uInt NeededLen = 0;
  deflateGetDictionary(&ZStreamIt->second.Z, Z_NULL, &NeededLen);

  uint8_t *Dictionary = nullptr;
  if (DictionaryPtr != 0) {
    BUFFER_CHECK(DictionaryBuf, MemInst, DictionaryPtr, NeededLen,
                 "WasmEdgeZlibDeflateGetDictionary")
    Dictionary = DictionaryBuf;
  }
  uint32_t *DictLength = nullptr;
  if (DictLengthPtr != 0) {
    PTR_CHECK(DictLengthOut, MemInst, DictLengthPtr, uint32_t,
              "WasmEdgeZlibDeflateGetDictionary")
    DictLength = DictLengthOut;
  }

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
  if (SourceZStreamIt->second.Kind != ZStreamKind::Deflate) {
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }
  auto *SourceZStream = &SourceZStreamIt->second.Z;
  const bool SourceGzipWrap = SourceZStreamIt->second.GzipWrap;
  const bool SourceRawDeflate = SourceZStreamIt->second.RawDeflate;
  const bool SourceDeflateStarted = SourceZStreamIt->second.DeflateStarted;

  MEMINST_CHECK(MemInst, Frame, 0)
  auto *SourceModuleZStream = MemInst->getPointer<WasmZStream *>(SourcePtr);
  auto *DestModuleZStream = MemInst->getPointer<WasmZStream *>(DestPtr);
  if (unlikely(SourceModuleZStream == nullptr ||
               DestModuleZStream == nullptr)) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibDeflateCopy] "sv
                  "Out-of-bounds ZStreamPtr received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const auto [It, Inserted] =
      Env.ZStreamMap.try_emplace(DestPtr, ZStreamKind::Deflate);
  if (unlikely(!Inserted)) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibDeflateCopy] "sv
                  "Destination stream is already live."sv);
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }

  const auto ZRes = SyncRun("WasmEdgeZlibDeflateCopy", Env, DestPtr, Frame,
                            [&](z_stream *DestZStream) {
                              return deflateCopy(DestZStream, SourceZStream);
                            });

  if (!ZRes || *ZRes != Z_OK) {
    It->second.Z.state = Z_NULL;
    Env.ZStreamMap.erase(It);
    return ZRes;
  }

  It->second.GzipWrap = SourceGzipWrap;
  It->second.RawDeflate = SourceRawDeflate;
  It->second.DeflateStarted = SourceDeflateStarted;

  const auto SourceHeaderIt = Env.GZHeaderMap.find(SourcePtr);
  if (SourceHeaderIt != Env.GZHeaderMap.end())
    Env.GZHeaderMap.insert_or_assign(DestPtr, SourceHeaderIt->second);

  DestModuleZStream->NextIn = SourceModuleZStream->NextIn;
  DestModuleZStream->AvailIn = SourceModuleZStream->AvailIn;
  DestModuleZStream->TotalIn = SourceModuleZStream->TotalIn;
  DestModuleZStream->NextOut = SourceModuleZStream->NextOut;
  DestModuleZStream->AvailOut = SourceModuleZStream->AvailOut;
  DestModuleZStream->TotalOut = SourceModuleZStream->TotalOut;
  DestModuleZStream->DataType = SourceModuleZStream->DataType;
  DestModuleZStream->Adler = SourceModuleZStream->Adler;
  DestModuleZStream->Reserved = SourceModuleZStream->Reserved;

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibDeflateReset::body(const Runtime::CallingFrame &Frame,
                               uint32_t ZStreamPtr) {

  if (streamKindMismatch(Env, ZStreamPtr, ZStreamKind::Deflate)) {
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }

  const auto ZRes =
      SyncRun("WasmEdgeZlibDeflateReset", Env, ZStreamPtr, Frame,
              [&](z_stream *HostZStream) { return deflateReset(HostZStream); });

  if (ZRes && *ZRes == Z_OK) {
    if (const auto It = Env.ZStreamMap.find(ZStreamPtr);
        It != Env.ZStreamMap.end()) {
      It->second.DeflateStarted = false;
    }
  }

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibDeflateParams::body(const Runtime::CallingFrame &Frame,
                                uint32_t ZStreamPtr, int32_t Level,
                                int32_t Strategy) {

  if (streamKindMismatch(Env, ZStreamPtr, ZStreamKind::Deflate)) {
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }

  const bool ParamsValid =
      (Level == Z_DEFAULT_COMPRESSION || (Level >= 0 && Level <= 9)) &&
      Strategy >= 0 && Strategy <= Z_FIXED;

  return SyncRun(
      "WasmEdgeZlibDeflateParams", Env, ZStreamPtr, Frame,
      [&](z_stream *HostZStream) {
        return deflateParams(HostZStream, Level, Strategy);
      },
      /*ValidateInputBuffer=*/ParamsValid,
      /*ValidateOutputBuffer=*/ParamsValid);
}

Expect<int32_t> WasmEdgeZlibDeflateTune::body(
    const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr, int32_t GoodLength,
    int32_t MaxLazy, int32_t NiceLength, int32_t MaxChain) {

  if (streamKindMismatch(Env, ZStreamPtr, ZStreamKind::Deflate)) {
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }

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

  if (streamKindMismatch(Env, ZStreamPtr, ZStreamKind::Deflate)) {
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }

  MEMINST_CHECK(MemInst, Frame, 0)

  uint32_t *Pending = nullptr;
  if (PendingPtr != 0) {
    PTR_CHECK(PendingOut, MemInst, PendingPtr, uint32_t,
              "WasmEdgeZlibDeflatePending")
    Pending = PendingOut;
  }
  int32_t *Bits = nullptr;
  if (BitsPtr != 0) {
    PTR_CHECK(BitsOut, MemInst, BitsPtr, int32_t, "WasmEdgeZlibDeflatePending")
    Bits = BitsOut;
  }

  return SyncRun("WasmEdgeZlibDeflatePending", Env, ZStreamPtr, Frame,
                 [&](z_stream *HostZStream) {
                   return deflatePending(HostZStream, Pending, Bits);
                 });
}

Expect<int32_t>
WasmEdgeZlibDeflatePrime::body(const Runtime::CallingFrame &Frame,
                               uint32_t ZStreamPtr, int32_t Bits,
                               int32_t Value) {

  if (streamKindMismatch(Env, ZStreamPtr, ZStreamKind::Deflate)) {
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }

  return SyncRun("WasmEdgeZlibDeflatePrime", Env, ZStreamPtr, Frame,
                 [&](z_stream *HostZStream) {
                   return deflatePrime(HostZStream, Bits, Value);
                 });
}

Expect<int32_t>
WasmEdgeZlibDeflateSetHeader::body(const Runtime::CallingFrame &Frame,
                                   uint32_t ZStreamPtr, uint32_t HeadPtr) {

  if (streamKindMismatch(Env, ZStreamPtr, ZStreamKind::Deflate)) {
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }

  MEMINST_CHECK(MemInst, Frame, 0)

  if (HeadPtr == 0) {
    const auto ZRes = SyncRun("WasmEdgeZlibDeflateSetHeader", Env, ZStreamPtr,
                              Frame, [&](z_stream *HostZStream) {
                                return deflateSetHeader(HostZStream, Z_NULL);
                              });
    if (ZRes && *ZRes == Z_OK)
      Env.GZHeaderMap.erase(ZStreamPtr);
    return ZRes;
  }

  {
    const auto StoreIt = Env.GZHeaderMap.find(ZStreamPtr);
    gz_header *CurrentHead = StoreIt != Env.GZHeaderMap.end()
                                 ? StoreIt->second->HostGZHeader.get()
                                 : Z_NULL;
    const auto ProbeRes =
        SyncRun("WasmEdgeZlibDeflateSetHeader", Env, ZStreamPtr, Frame,
                [&](z_stream *HostZStream) {
                  return deflateSetHeader(HostZStream, CurrentHead);
                });
    if (!ProbeRes || *ProbeRes != Z_OK) {
      return ProbeRes;
    }
  }

  auto *ModuleGZHeader = MemInst->getPointer<WasmGZHeader *>(HeadPtr);
  if (unlikely(ModuleGZHeader == nullptr)) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibDeflateSetHeader] "sv
                  "Out-of-bounds gzip header."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto Store = std::make_shared<WasmEdgeZlibEnvironment::GZStore>();
  Store->WasmGZHeaderOffset = HeadPtr;
  Store->IsInflate = false;
  Store->HostGZHeader = std::make_unique<gz_header>();

  const uint32_t ExtraLen16 = ModuleGZHeader->ExtraLen & UINT32_C(0xffff);
  const bool HasExtra = ModuleGZHeader->Extra != 0;
  const bool HasName = ModuleGZHeader->Name != 0;
  const bool HasComment = ModuleGZHeader->Comment != 0;

  if (HasExtra) {
    if (ExtraLen16 != 0) {
      const auto ExtraSpan =
          MemInst->getSpan<Bytef>(ModuleGZHeader->Extra, ExtraLen16);
      if (unlikely(ExtraSpan.data() == nullptr)) {
        spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibDeflateSetHeader] "sv
                      "Out-of-bounds gzip header extra field."sv);
        return Unexpect(ErrCode::Value::HostFuncError);
      }
      Store->Extra.assign(ExtraSpan.begin(), ExtraSpan.end());
    } else {
      Store->Extra.assign(1, 0);
    }
  }
  if (HasName) {
    const auto NameStr = getBoundedInBoundsCString(
        *MemInst, ModuleGZHeader->Name, MaxGZHeaderStringLen);
    if (unlikely(!NameStr)) {
      spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibDeflateSetHeader] "sv
                    "Out-of-bounds gzip header name field."sv);
      return Unexpect(ErrCode::Value::HostFuncError);
    }
    Store->Name.assign(NameStr->data(), NameStr->size());
  }
  if (HasComment) {
    const auto CommentStr = getBoundedInBoundsCString(
        *MemInst, ModuleGZHeader->Comment, MaxGZHeaderStringLen);
    if (unlikely(!CommentStr)) {
      spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibDeflateSetHeader] "sv
                    "Out-of-bounds gzip header comment field."sv);
      return Unexpect(ErrCode::Value::HostFuncError);
    }
    Store->Comment.assign(CommentStr->data(), CommentStr->size());
  }

  Store->HostGZHeader->text = ModuleGZHeader->Text;
  Store->HostGZHeader->time = ModuleGZHeader->Time;
  Store->HostGZHeader->xflags = ModuleGZHeader->XFlags;
  Store->HostGZHeader->os = ModuleGZHeader->OS;
  Store->HostGZHeader->hcrc = ModuleGZHeader->HCRC;
  Store->HostGZHeader->extra_len = ExtraLen16;

  auto *HostGZHeaderPtr = Store->HostGZHeader.get();
  HostGZHeaderPtr->extra = HasExtra ? Store->Extra.data() : Z_NULL;
  HostGZHeaderPtr->name =
      HasName ? reinterpret_cast<Bytef *>(Store->Name.data()) : Z_NULL;
  HostGZHeaderPtr->comment =
      HasComment ? reinterpret_cast<Bytef *>(Store->Comment.data()) : Z_NULL;

  const auto ZRes =
      SyncRun("WasmEdgeZlibDeflateSetHeader", Env, ZStreamPtr, Frame,
              [&](z_stream *HostZStream) {
                return deflateSetHeader(HostZStream, HostGZHeaderPtr);
              });

  if (ZRes && *ZRes == Z_OK)
    Env.GZHeaderMap.insert_or_assign(ZStreamPtr, std::move(Store));

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibInflateInit2::body(const Runtime::CallingFrame &Frame,
                               uint32_t ZStreamPtr, int32_t WindowBits) {
  const auto ZRes =
      initStream("WasmEdgeZlibInflateInit2", Env, ZStreamPtr, Frame,
                 ZStreamKind::Inflate, [&](z_stream *HostZStream) {
                   return inflateInit2(HostZStream, WindowBits);
                 });
  if (ZRes && *ZRes == Z_OK) {
    if (const auto It = Env.ZStreamMap.find(ZStreamPtr);
        It != Env.ZStreamMap.end()) {
      It->second.RawInflate = WindowBits < 0;
      It->second.GzipWrap = WindowBits >= 16;
    }
  }
  return ZRes;
}

Expect<int32_t> WasmEdgeZlibInflateSetDictionary::body(
    const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
    uint32_t DictionaryPtr, uint32_t DictLength) {

  if (const auto It = Env.ZStreamMap.find(ZStreamPtr);
      It != Env.ZStreamMap.end() &&
      (It->second.Kind != ZStreamKind::Inflate ||
       (!It->second.RawInflate && !It->second.MayNeedDict))) {
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }

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
  if (ZStreamIt->second.Kind != ZStreamKind::Inflate) {
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }

  uInt NeededLen = 0;
  inflateGetDictionary(&ZStreamIt->second.Z, Z_NULL, &NeededLen);

  uint8_t *Dictionary = nullptr;
  if (DictionaryPtr != 0) {
    BUFFER_CHECK(DictionaryBuf, MemInst, DictionaryPtr, NeededLen,
                 "WasmEdgeZlibInflateGetDictionary")
    Dictionary = DictionaryBuf;
  }
  uint32_t *DictLength = nullptr;
  if (DictLengthPtr != 0) {
    PTR_CHECK(DictLengthOut, MemInst, DictLengthPtr, uint32_t,
              "WasmEdgeZlibInflateGetDictionary")
    DictLength = DictLengthOut;
  }

  return SyncRun("WasmEdgeZlibInflateGetDictionary", Env, ZStreamPtr, Frame,
                 [&](z_stream *HostZStream) {
                   return inflateGetDictionary(HostZStream, Dictionary,
                                               DictLength);
                 });
}

Expect<int32_t>
WasmEdgeZlibInflateSync::body(const Runtime::CallingFrame &Frame,
                              uint32_t ZStreamPtr) {

  if (streamKindMismatch(Env, ZStreamPtr, ZStreamKind::Inflate)) {
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }

  return SyncRun(
      "WasmEdgeZlibInflateSync", Env, ZStreamPtr, Frame,
      [&](z_stream *HostZStream) { return inflateSync(HostZStream); },
      /*ValidateInputBuffer=*/true);
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
  if (SourceZStreamIt->second.Kind != ZStreamKind::Inflate) {
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }
  auto *SourceZStream = &SourceZStreamIt->second.Z;
  const bool SourceGzipWrap = SourceZStreamIt->second.GzipWrap;
  const bool SourceRawInflate = SourceZStreamIt->second.RawInflate;
  const bool SourceMayNeedDict = SourceZStreamIt->second.MayNeedDict;

  MEMINST_CHECK(MemInst, Frame, 0)
  auto *SourceModuleZStream = MemInst->getPointer<WasmZStream *>(SourcePtr);
  auto *DestModuleZStream = MemInst->getPointer<WasmZStream *>(DestPtr);
  if (unlikely(SourceModuleZStream == nullptr ||
               DestModuleZStream == nullptr)) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibInflateCopy] "sv
                  "Out-of-bounds ZStreamPtr received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const auto [It, Inserted] =
      Env.ZStreamMap.try_emplace(DestPtr, ZStreamKind::Inflate);
  if (unlikely(!Inserted)) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibInflateCopy] "sv
                  "Destination stream is already live."sv);
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }

  const auto ZRes = SyncRun("WasmEdgeZlibInflateCopy", Env, DestPtr, Frame,
                            [&](z_stream *DestZStream) {
                              return inflateCopy(DestZStream, SourceZStream);
                            });

  if (!ZRes || *ZRes != Z_OK) {
    It->second.Z.state = Z_NULL;
    Env.ZStreamMap.erase(It);
    return ZRes;
  }

  It->second.GzipWrap = SourceGzipWrap;
  It->second.RawInflate = SourceRawInflate;
  It->second.MayNeedDict = SourceMayNeedDict;

  const auto SourceHeaderIt = Env.GZHeaderMap.find(SourcePtr);
  if (SourceHeaderIt != Env.GZHeaderMap.end())
    Env.GZHeaderMap.insert_or_assign(DestPtr, SourceHeaderIt->second);

  DestModuleZStream->NextIn = SourceModuleZStream->NextIn;
  DestModuleZStream->AvailIn = SourceModuleZStream->AvailIn;
  DestModuleZStream->TotalIn = SourceModuleZStream->TotalIn;
  DestModuleZStream->NextOut = SourceModuleZStream->NextOut;
  DestModuleZStream->AvailOut = SourceModuleZStream->AvailOut;
  DestModuleZStream->TotalOut = SourceModuleZStream->TotalOut;
  DestModuleZStream->DataType = SourceModuleZStream->DataType;
  DestModuleZStream->Adler = SourceModuleZStream->Adler;
  DestModuleZStream->Reserved = SourceModuleZStream->Reserved;

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibInflateReset::body(const Runtime::CallingFrame &Frame,
                               uint32_t ZStreamPtr) {

  if (streamKindMismatch(Env, ZStreamPtr, ZStreamKind::Inflate)) {
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }

  const auto ZRes =
      SyncRun("WasmEdgeZlibInflateReset", Env, ZStreamPtr, Frame,
              [&](z_stream *HostZStream) { return inflateReset(HostZStream); });

  if (ZRes && *ZRes == Z_OK) {
    Env.GZHeaderMap.erase(ZStreamPtr);
    if (const auto It = Env.ZStreamMap.find(ZStreamPtr);
        It != Env.ZStreamMap.end()) {
      It->second.MayNeedDict = false;
    }
  }

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibInflateReset2::body(const Runtime::CallingFrame &Frame,
                                uint32_t ZStreamPtr, int32_t WindowBits) {

  if (streamKindMismatch(Env, ZStreamPtr, ZStreamKind::Inflate)) {
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }

  const auto ZRes = SyncRun("WasmEdgeZlibInflateReset2", Env, ZStreamPtr, Frame,
                            [&](z_stream *HostZStream) {
                              return inflateReset2(HostZStream, WindowBits);
                            });

  if (ZRes && *ZRes == Z_OK) {
    Env.GZHeaderMap.erase(ZStreamPtr);
    if (const auto It = Env.ZStreamMap.find(ZStreamPtr);
        It != Env.ZStreamMap.end()) {
      It->second.RawInflate = WindowBits < 0;
      It->second.GzipWrap = WindowBits >= 16;
      It->second.MayNeedDict = false;
    }
  }

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibInflatePrime::body(const Runtime::CallingFrame &Frame,
                               uint32_t ZStreamPtr, int32_t Bits,
                               int32_t Value) {

  if (streamKindMismatch(Env, ZStreamPtr, ZStreamKind::Inflate)) {
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }

  return SyncRun("WasmEdgeZlibInflatePrime", Env, ZStreamPtr, Frame,
                 [&](z_stream *HostZStream) {
                   return inflatePrime(HostZStream, Bits, Value);
                 });
}

Expect<int32_t>
WasmEdgeZlibInflateMark::body(const Runtime::CallingFrame &Frame,
                              uint32_t ZStreamPtr) {

  if (streamKindMismatch(Env, ZStreamPtr, ZStreamKind::Inflate)) {
    return INT32_C(-65536);
  }

  return SyncRun(
      "WasmEdgeZlibInflateMark", Env, ZStreamPtr, Frame,
      [&](z_stream *HostZStream) { return inflateMark(HostZStream); });
}

Expect<int32_t>
WasmEdgeZlibInflateGetHeader::body(const Runtime::CallingFrame &Frame,
                                   uint32_t ZStreamPtr, uint32_t HeadPtr) {

  if (const auto It = Env.ZStreamMap.find(ZStreamPtr);
      It != Env.ZStreamMap.end() &&
      (It->second.Kind != ZStreamKind::Inflate || !It->second.GzipWrap)) {
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }

  MEMINST_CHECK(MemInst, Frame, 0)

  if (unlikely(MemInst->getPointer<WasmGZHeader *>(HeadPtr) == nullptr)) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibInflateGetHeader] "sv
                  "Out-of-bounds gzip header."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto Store = std::make_shared<WasmEdgeZlibEnvironment::GZStore>();
  Store->WasmGZHeaderOffset = HeadPtr;
  Store->IsInflate = true;
  Store->HostGZHeader = std::make_unique<gz_header>();
  auto *HostGZHeaderPtr = Store->HostGZHeader.get();

  const auto ZRes =
      SyncRun("WasmEdgeZlibInflateGetHeader", Env, ZStreamPtr, Frame,
              [&](z_stream *HostZStream) {
                return inflateGetHeader(HostZStream, HostGZHeaderPtr);
              });

  if (ZRes && *ZRes == Z_OK) {
    if (auto *ModuleGZHeader = MemInst->getPointer<WasmGZHeader *>(HeadPtr);
        ModuleGZHeader != nullptr) {
      ModuleGZHeader->Done = 0;
    }
    Env.GZHeaderMap.insert_or_assign(ZStreamPtr, std::move(Store));
  }

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibInflateBackInit::body(const Runtime::CallingFrame &Frame,
                                  uint32_t ZStreamPtr, int32_t WindowBits,
                                  uint32_t WindowPtr) {
  MEMINST_CHECK(MemInst, Frame, 0)

  uint8_t *Window = nullptr;
  if (WindowPtr != 0 && WindowBits >= 8 && WindowBits <= 15) {
    const auto WindowSpan =
        MemInst->getSpan<uint8_t>(WindowPtr, UINT64_C(1) << WindowBits);
    if (unlikely(WindowSpan.data() == nullptr)) {
      spdlog::error("[WasmEdge-Zlib] [InflateBackInit] "sv
                    "Out-of-bounds window buffer."sv);
      return Unexpect(ErrCode::Value::HostFuncError);
    }
    Window = WindowSpan.data();
  } else {
    Window = nullptr;
  }

  return initStream("WasmEdgeZlibInflateBackInit", Env, ZStreamPtr, Frame,
                    ZStreamKind::InflateBack, [&](z_stream *HostZStream) {
                      return inflateBackInit(HostZStream, WindowBits, Window);
                    });
}

Expect<int32_t>
WasmEdgeZlibInflateBackEnd::body(const Runtime::CallingFrame &Frame,
                                 uint32_t ZStreamPtr) {

  if (streamKindMismatch(Env, ZStreamPtr, ZStreamKind::InflateBack)) {
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }

  const auto ZRes = SyncRun(
      "WasmEdgeZlibInflateBackEnd", Env, ZStreamPtr, Frame,
      [&](z_stream *HostZStream) { return inflateBackEnd(HostZStream); });

  if (ZRes && *ZRes != Z_STREAM_ERROR)
    Env.ZStreamMap.erase(ZStreamPtr);

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibZlibCompilerFlags::body(const Runtime::CallingFrame &) {
  return zlibCompileFlags();
}

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
  *SourceLen = static_cast<uint32_t>(HostSourceLen);
  *DestLen = static_cast<uint32_t>(HostDestLen);

  return ZRes;
}

static WASI::Environ *
getWasiEnviron(const Runtime::CallingFrame &Frame) noexcept {
  const auto *WasiMod = Frame.getWASIModule();
  if (WasiMod == nullptr) {
    return nullptr;
  }
  const auto *Typed = dynamic_cast<const WasiModule *>(WasiMod);
  if (Typed == nullptr) {
    return nullptr;
  }
  return const_cast<WASI::Environ *>(Typed->getEnv());
}

static void releaseOwnedWasiFd(const Runtime::CallingFrame &Frame,
                               int64_t OwnedWasiFd) noexcept {
  if (OwnedWasiFd < 0) {
    return;
  }
  if (auto *WasiEnv = getWasiEnviron(Frame); WasiEnv != nullptr) {
    WasiEnv->fdClose(static_cast<__wasi_fd_t>(OwnedWasiFd));
  }
}

#if defined(_WIN32)
// TODO(zlib): Bridge Windows HANDLEs to CRT descriptors.
static int dupNativeHandle(uint64_t) noexcept { return -1; }
static void closeNativeFd(int) noexcept {}
static int64_t tellNativeFd(int) noexcept { return -1; }
static void seekNativeFd(int, int64_t) noexcept {}
#else
static int dupNativeHandle(uint64_t Native) noexcept {
  return ::fcntl(static_cast<int>(Native), F_DUPFD_CLOEXEC, 0);
}
static void closeNativeFd(int Fd) noexcept { ::close(Fd); }
static int64_t tellNativeFd(int Fd) noexcept {
  return static_cast<int64_t>(::lseek(Fd, 0, SEEK_CUR));
}
static void seekNativeFd(int Fd, int64_t Offset) noexcept {
  ::lseek(Fd, static_cast<off_t>(Offset), SEEK_SET);
}
#endif

enum class GZOpenDir { None, Read, Write, Append };
enum class GZOpenDirect { Auto, Transparent, ForceGzip };

struct GZOpenMode {
  GZOpenDir Dir = GZOpenDir::None;
  bool Exclusive = false;
  bool NonBlock = false;
  GZOpenDirect Direct = GZOpenDirect::Auto;
};

static std::optional<GZOpenMode>
parseGZOpenMode(std::string_view Mode) noexcept {
  GZOpenMode Parsed;
  for (const char C : Mode) {
    switch (C) {
    case 'r':
      Parsed.Dir = GZOpenDir::Read;
      break;
    case 'w':
      Parsed.Dir = GZOpenDir::Write;
      break;
    case 'a':
      Parsed.Dir = GZOpenDir::Append;
      break;
    case 'x':
      Parsed.Exclusive = true;
      break;
    case 'N':
      Parsed.NonBlock = true;
      break;
    case 'T':
      Parsed.Direct = GZOpenDirect::Transparent;
      break;
    case 'G':
      Parsed.Direct = GZOpenDirect::ForceGzip;
      break;
    case '+':
      return std::nullopt;
    default:
      break;
    }
  }
  if (Parsed.Dir == GZOpenDir::None) {
    return std::nullopt;
  }
  if (Parsed.Dir == GZOpenDir::Read &&
      Parsed.Direct == GZOpenDirect::Transparent) {
    return std::nullopt;
  }
  if (Parsed.Dir != GZOpenDir::Read &&
      Parsed.Direct == GZOpenDirect::ForceGzip) {
    return std::nullopt;
  }
  return Parsed;
}

static WasmEdgeZlibEnvironment::GZFileEntry
wasiFdToGZ(WASI::Environ &WasiEnv, __wasi_fd_t WasiFd, const char *Mode,
           bool OwnWasiFd) noexcept {
  const auto Parsed = parseGZOpenMode(Mode);
  if (!Parsed) {
    spdlog::error("[WasmEdge-Zlib] [wasiFdToGZ] "sv
                  "Unsupported gz mode \"{}\"."sv,
                  Mode);
    if (OwnWasiFd) {
      WasiEnv.fdClose(WasiFd);
    }
    return {};
  }
  const __wasi_rights_t RequiredRights = Parsed->Dir == GZOpenDir::Read
                                             ? __WASI_RIGHTS_FD_READ
                                             : __WASI_RIGHTS_FD_WRITE;
  if (!WasiEnv.canFd(WasiFd, RequiredRights)) {
    spdlog::error("[WasmEdge-Zlib] [wasiFdToGZ] "sv
                  "WASI fd lacks the rights required by mode \"{}\"."sv,
                  Mode);
    if (OwnWasiFd) {
      WasiEnv.fdClose(WasiFd);
    }
    return {};
  }
  const bool CanSeek = WasiEnv.canFd(WasiFd, __WASI_RIGHTS_FD_SEEK);
  const bool CanTell = WasiEnv.canFd(WasiFd, __WASI_RIGHTS_FD_TELL);
  if (Parsed->Dir == GZOpenDir::Append && !CanSeek) {
    __wasi_fdstat_t FdStat;
    const bool AppendOnly = WasiEnv.fdFdstatGet(WasiFd, FdStat).has_value() &&
                            (FdStat.fs_flags & __WASI_FDFLAGS_APPEND) != 0;
    if (!AppendOnly) {
      spdlog::error("[WasmEdge-Zlib] [wasiFdToGZ] "sv
                    "Append mode needs FD_SEEK on a non-append-only fd."sv);
      if (OwnWasiFd) {
        WasiEnv.fdClose(WasiFd);
      }
      return {};
    }
  }
  auto HandleRes = WasiEnv.getNativeHandler(WasiFd);
  if (!HandleRes) {
    if (OwnWasiFd) {
      WasiEnv.fdClose(WasiFd);
    }
    return {};
  }
  const int DupFd = dupNativeHandle(*HandleRes);
  if (OwnWasiFd) {
    WasiEnv.fdClose(WasiFd);
  }
  if (DupFd < 0) {
    spdlog::error("[WasmEdge-Zlib] [wasiFdToGZ] "sv
                  "Failed to duplicate the host descriptor for zlib."sv);
    return {};
  }
  const bool RestoreOffset =
      !OwnWasiFd && Parsed->Dir == GZOpenDir::Append && !CanSeek;
  const int64_t SavedOffset = RestoreOffset ? tellNativeFd(DupFd) : -1;
  gzFile GZ = gzdopen(DupFd, Mode);
  bool RestoredAppendOffset = false;
  if (RestoreOffset && SavedOffset >= 0) {
    seekNativeFd(DupFd, SavedOffset);
    RestoredAppendOffset = true;
  }
  if (GZ == nullptr) {
    closeNativeFd(DupFd);
    return {};
  }
  return {GZ, CanSeek, CanTell, RestoredAppendOffset,
          Parsed->Dir == GZOpenDir::Read};
}

static WasmEdgeZlibEnvironment::GZFileEntry
wasiGZOpen(WASI::Environ &WasiEnv, const char *Path,
           const char *Mode) noexcept {
  const auto Parsed = parseGZOpenMode(Mode);
  if (!Parsed) {
    return {};
  }
#if defined(_WIN32)
  static_cast<void>(WasiEnv);
  static_cast<void>(Path);
  spdlog::error("[WasmEdge-Zlib] [wasiGZOpen] "sv
                "WASI-mediated gzopen is not supported on Windows yet; "sv
                "returning a null handle for mode \"{}\"."sv,
                Mode);
  return {};
#else
  auto OpenFlags = static_cast<__wasi_oflags_t>(0);
  auto FdFlags = static_cast<__wasi_fdflags_t>(0);
  __wasi_rights_t Rights = static_cast<__wasi_rights_t>(0);
  switch (Parsed->Dir) {
  case GZOpenDir::Read:
    Rights = __WASI_RIGHTS_FD_READ;
    break;
  case GZOpenDir::Write:
    OpenFlags = __WASI_OFLAGS_CREAT | __WASI_OFLAGS_TRUNC;
    Rights = __WASI_RIGHTS_FD_WRITE;
    break;
  case GZOpenDir::Append:
    OpenFlags = __WASI_OFLAGS_CREAT;
    FdFlags = __WASI_FDFLAGS_APPEND;
    Rights = __WASI_RIGHTS_FD_WRITE;
    break;
  case GZOpenDir::None:
    return {};
  }
  if (Parsed->NonBlock) {
    FdFlags = static_cast<__wasi_fdflags_t>(FdFlags | __WASI_FDFLAGS_NONBLOCK);
  }
  __wasi_rights_t OptionalRights =
      __WASI_RIGHTS_FD_SEEK | __WASI_RIGHTS_FD_TELL;
  __wasi_fdstat_t DirStat;
  if (WasiEnv.fdFdstatGet(3, DirStat).has_value()) {
    __wasi_rights_t Inheriting = DirStat.fs_rights_inheriting;
    if ((Inheriting & __WASI_RIGHTS_FD_SEEK) != 0) {
      Inheriting |= __WASI_RIGHTS_FD_TELL;
    }
    OptionalRights &= Inheriting;
  }
  Rights |= OptionalRights;
  auto LookupFlags = __WASI_LOOKUPFLAGS_SYMLINK_FOLLOW;
  if (Parsed->Exclusive && Parsed->Dir != GZOpenDir::Read) {
    OpenFlags |= __WASI_OFLAGS_EXCL;
    LookupFlags = static_cast<__wasi_lookupflags_t>(0);
  }
  while (Path[0] == '/' || (Path[0] == '.' && Path[1] == '/') ||
         (Path[0] == '.' && Path[1] == '\0')) {
    Path += (Path[0] == '.' && Path[1] == '/') ? 2 : 1;
  }
  auto FdRes = WasiEnv.pathOpen(3, Path, LookupFlags, OpenFlags, Rights, Rights,
                                FdFlags);
  if (!FdRes) {
    return {};
  }
  return wasiFdToGZ(WasiEnv, *FdRes, Mode, /*OwnWasiFd=*/true);
#endif
}

constexpr uint64_t MaxGZOpenPathLen = 4096;
constexpr uint64_t MaxGZOpenModeLen = 63;

Expect<uint32_t> WasmEdgeZlibGZOpen::body(const Runtime::CallingFrame &Frame,
                                          uint32_t PathPtr, uint32_t ModePtr) {
  MEMINST_CHECK(MemInst, Frame, 0)

  if (PathPtr == 0) {
    return UINT32_C(0);
  }

  const auto PathStr =
      getBoundedInBoundsCString(*MemInst, PathPtr, MaxGZOpenPathLen);
  const auto ModeStr =
      getBoundedInBoundsCString(*MemInst, ModePtr, MaxGZOpenModeLen);
  if (unlikely(!PathStr || !ModeStr)) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZOpen] "sv
                  "Out-of-bounds or oversized path or mode string."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  const std::string Path(PathStr->data(), PathStr->size());
  const std::string Mode(ModeStr->data(), ModeStr->size());

  WasmEdgeZlibEnvironment::GZFileEntry Entry{};
  if (Frame.getWASIModule() == nullptr) {
    Entry = {gzopen(Path.c_str(), Mode.c_str()), /*CanSeek=*/true,
             /*CanTell=*/true, /*RestoredAppendOffset=*/false};
  } else if (auto *WasiEnv = getWasiEnviron(Frame); WasiEnv != nullptr) {
    Entry = wasiGZOpen(*WasiEnv, Path.c_str(), Mode.c_str());
  } else {
    spdlog::error(
        "[WasmEdge-Zlib] [WasmEdgeZlibGZOpen] "sv
        "A WASI module is configured but is not one this plugin can "sv
        "read capabilities from; refusing to open outside the "sv
        "capability layer."sv);
    return UINT32_C(0);
  }
  if (unlikely(Entry.GZ == nullptr)) {
    return UINT32_C(0);
  }

  const uint32_t NewWasmGZFile = Env.NextGZFile++;
  Env.GZFileMap.emplace(NewWasmGZFile, Entry);

  return NewWasmGZFile;
}

Expect<uint32_t> WasmEdgeZlibGZDOpen::body(const Runtime::CallingFrame &,
                                           int32_t, uint32_t) {
  // TODO(zlib): Re-enable gzdopen after WASI fd ownership is safe at teardown.
  return UINT32_C(0);
}

Expect<int32_t> WasmEdgeZlibGZBuffer::body(const Runtime::CallingFrame &,
                                           uint32_t GZFile, uint32_t Size) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZBuffer] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  if (Size >= UINT32_C(0x80000000)) {
    return INT32_C(-1);
  }
  constexpr uint32_t MaxGZBufferSize = UINT32_C(64) * 1024 * 1024;
  return gzbuffer(GZFileIt->second.GZ,
                  Size > MaxGZBufferSize ? MaxGZBufferSize : Size);
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

  return gzsetparams(GZFileIt->second.GZ, Level, Strategy);
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

  if (Len > UINT32_C(0x7fffffff)) {
    return gzread(GZFileIt->second.GZ, nullptr, Len);
  }

  BUFFER_CHECK(Buf, MemInst, BufPtr, Len, "WasmEdgeZlibGZRead")

  return gzread(GZFileIt->second.GZ, Buf, Len);
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
  if (Bytes > UINT32_MAX) {
    return gzfread(nullptr, static_cast<z_size_t>(-1), 2, GZFileIt->second.GZ);
  }
  BUFFER_CHECK(Buf, MemInst, BufPtr, Bytes, "WasmEdgeZlibGZFread")

  return gzfread(Buf, Size, NItems, GZFileIt->second.GZ);
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

  if (Len > UINT32_C(0x7fffffff)) {
    return gzwrite(GZFileIt->second.GZ, nullptr, Len);
  }

  BUFFER_CHECK(Buf, MemInst, BufPtr, Len, "WasmEdgeZlibGZWrite")

  return gzwrite(GZFileIt->second.GZ, Buf, Len);
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
  if (Bytes > UINT32_MAX) {
    return gzfwrite(nullptr, static_cast<z_size_t>(-1), 2, GZFileIt->second.GZ);
  }
  BUFFER_CHECK(Buf, MemInst, BufPtr, Bytes, "WasmEdgeZlibGZFwrite")

  return gzfwrite(Buf, Size, NItems, GZFileIt->second.GZ);
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

  const auto String =
      getBoundedInBoundsCString(*MemInst, StringPtr, UINT64_MAX);
  if (unlikely(!String)) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZPuts] "sv
                  "Out-of-bounds string."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  if (String->empty()) {
    return gzputs(GZFileIt->second.GZ, "");
  }
  const int Written = gzwrite(GZFileIt->second.GZ, String->data(),
                              static_cast<unsigned>(String->size()));
  return Written > 0 && static_cast<size_t>(Written) == String->size() ? Written
                                                                       : -1;
}

Expect<int32_t> WasmEdgeZlibGZPutc::body(const Runtime::CallingFrame &,
                                         uint32_t GZFile, int32_t C) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZPutc] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return gzputc(GZFileIt->second.GZ, C);
}

Expect<int32_t> WasmEdgeZlibGZGetc::body(const Runtime::CallingFrame &,
                                         uint32_t GZFile) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZGetc] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return gzgetc(GZFileIt->second.GZ);
}

Expect<int32_t> WasmEdgeZlibGZUngetc::body(const Runtime::CallingFrame &,
                                           int32_t C, uint32_t GZFile) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZUngetc] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return gzungetc(C, GZFileIt->second.GZ);
}

Expect<int32_t> WasmEdgeZlibGZFlush::body(const Runtime::CallingFrame &,
                                          uint32_t GZFile, int32_t Flush) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZFlush] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return gzflush(GZFileIt->second.GZ, Flush);
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

  if (unlikely(!GZFileIt->second.CanSeek && GZFileIt->second.OpenedForRead)) {
    return static_cast<int32_t>(-1);
  }

  return gzseek(GZFileIt->second.GZ, Offset, Whence);
}

Expect<int32_t> WasmEdgeZlibGZRewind::body(const Runtime::CallingFrame &,
                                           uint32_t GZFile) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZRewind] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  if (unlikely(!GZFileIt->second.CanSeek && GZFileIt->second.OpenedForRead)) {
    return static_cast<int32_t>(-1);
  }

  return gzrewind(GZFileIt->second.GZ);
}

Expect<int32_t> WasmEdgeZlibGZTell::body(const Runtime::CallingFrame &,
                                         uint32_t GZFile) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZTell] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return gztell(GZFileIt->second.GZ);
}

Expect<int32_t> WasmEdgeZlibGZOffset::body(const Runtime::CallingFrame &,
                                           uint32_t GZFile) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZOffset] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  if (unlikely(!GZFileIt->second.CanTell ||
               GZFileIt->second.RestoredAppendOffset)) {
    return static_cast<int32_t>(-1);
  }

  return gzoffset(GZFileIt->second.GZ);
}

Expect<int32_t> WasmEdgeZlibGZEof::body(const Runtime::CallingFrame &,
                                        uint32_t GZFile) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZEof] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return gzeof(GZFileIt->second.GZ);
}

Expect<int32_t> WasmEdgeZlibGZDirect::body(const Runtime::CallingFrame &,
                                           uint32_t GZFile) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZDirect] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return gzdirect(GZFileIt->second.GZ);
}

Expect<int32_t> WasmEdgeZlibGZClose::body(const Runtime::CallingFrame &Frame,
                                          uint32_t GZFile) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZClose] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const int64_t OwnedWasiFd = GZFileIt->second.OwnedWasiFd;
  const auto ZRes = gzclose(GZFileIt->second.GZ);

  Env.GZFileMap.erase(GZFileIt);
  releaseOwnedWasiFd(Frame, OwnedWasiFd);

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibGZClose_r::body(const Runtime::CallingFrame &Frame,
                                            uint32_t GZFile) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZClose_r] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const int64_t OwnedWasiFd = GZFileIt->second.OwnedWasiFd;
  const auto ZRes = gzclose_r(GZFileIt->second.GZ);

  if (ZRes != Z_STREAM_ERROR) {
    Env.GZFileMap.erase(GZFileIt);
    releaseOwnedWasiFd(Frame, OwnedWasiFd);
  }

  return ZRes;
}

Expect<int32_t> WasmEdgeZlibGZClose_w::body(const Runtime::CallingFrame &Frame,
                                            uint32_t GZFile) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZClose_w] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const int64_t OwnedWasiFd = GZFileIt->second.OwnedWasiFd;
  const auto ZRes = gzclose_w(GZFileIt->second.GZ);

  if (ZRes != Z_STREAM_ERROR) {
    Env.GZFileMap.erase(GZFileIt);
    releaseOwnedWasiFd(Frame, OwnedWasiFd);
  }

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

  gzclearerr(GZFileIt->second.GZ);

  return Expect<void>{};
}

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

  const auto WasmZlibVersion =
      getBoundedInBoundsCString(*MemInst, VersionPtr, MaxZlibVersionLen);
  if (!WasmZlibVersion || !CheckVersion(WasmZlibVersion->data())) {
    return static_cast<int32_t>(Z_VERSION_ERROR);
  }

  return initStream("WasmEdgeZlibDeflateInit_", Env, ZStreamPtr, Frame,
                    ZStreamKind::Deflate, [&](z_stream *HostZStream) {
                      return deflateInit_(HostZStream, Level,
                                          WasmZlibVersion->data(),
                                          sizeof(z_stream));
                    });
}

Expect<int32_t>
WasmEdgeZlibInflateInit_::body(const Runtime::CallingFrame &Frame,
                               uint32_t ZStreamPtr, uint32_t VersionPtr,
                               int32_t StreamSize) {
  if (!CheckSize(StreamSize))
    return static_cast<int32_t>(Z_VERSION_ERROR);

  MEMINST_CHECK(MemInst, Frame, 0)

  const auto WasmZlibVersion =
      getBoundedInBoundsCString(*MemInst, VersionPtr, MaxZlibVersionLen);
  if (!WasmZlibVersion || !CheckVersion(WasmZlibVersion->data())) {
    return static_cast<int32_t>(Z_VERSION_ERROR);
  }

  return initStream("WasmEdgeZlibInflateInit_", Env, ZStreamPtr, Frame,
                    ZStreamKind::Inflate, [&](z_stream *HostZStream) {
                      return inflateInit_(HostZStream, WasmZlibVersion->data(),
                                          sizeof(z_stream));
                    });
}

Expect<int32_t> WasmEdgeZlibDeflateInit2_::body(
    const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr, int32_t Level,
    int32_t Method, int32_t WindowBits, int32_t MemLevel, int32_t Strategy,
    uint32_t VersionPtr, int32_t StreamSize) {
  if (!CheckSize(StreamSize))
    return static_cast<int32_t>(Z_VERSION_ERROR);

  MEMINST_CHECK(MemInst, Frame, 0)

  const auto WasmZlibVersion =
      getBoundedInBoundsCString(*MemInst, VersionPtr, MaxZlibVersionLen);
  if (!WasmZlibVersion || !CheckVersion(WasmZlibVersion->data())) {
    return static_cast<int32_t>(Z_VERSION_ERROR);
  }

  const auto ZRes =
      initStream("WasmEdgeZlibDeflateInit2_", Env, ZStreamPtr, Frame,
                 ZStreamKind::Deflate, [&](z_stream *HostZStream) {
                   return deflateInit2_(
                       HostZStream, Level, Method, WindowBits, MemLevel,
                       Strategy, WasmZlibVersion->data(), sizeof(z_stream));
                 });
  if (ZRes && *ZRes == Z_OK) {
    if (const auto It = Env.ZStreamMap.find(ZStreamPtr);
        It != Env.ZStreamMap.end()) {
      It->second.GzipWrap = WindowBits >= 16;
      It->second.RawDeflate = WindowBits < 0;
    }
  }
  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibInflateInit2_::body(const Runtime::CallingFrame &Frame,
                                uint32_t ZStreamPtr, int32_t WindowBits,
                                uint32_t VersionPtr, int32_t StreamSize) {
  if (!CheckSize(StreamSize))
    return static_cast<int32_t>(Z_VERSION_ERROR);

  MEMINST_CHECK(MemInst, Frame, 0)

  const auto WasmZlibVersion =
      getBoundedInBoundsCString(*MemInst, VersionPtr, MaxZlibVersionLen);
  if (!WasmZlibVersion || !CheckVersion(WasmZlibVersion->data())) {
    return static_cast<int32_t>(Z_VERSION_ERROR);
  }

  const auto ZRes = initStream(
      "WasmEdgeZlibInflateInit2_", Env, ZStreamPtr, Frame, ZStreamKind::Inflate,
      [&](z_stream *HostZStream) {
        return inflateInit2_(HostZStream, WindowBits, WasmZlibVersion->data(),
                             sizeof(z_stream));
      });
  if (ZRes && *ZRes == Z_OK) {
    if (const auto It = Env.ZStreamMap.find(ZStreamPtr);
        It != Env.ZStreamMap.end()) {
      It->second.RawInflate = WindowBits < 0;
      It->second.GzipWrap = WindowBits >= 16;
    }
  }
  return ZRes;
}

Expect<int32_t> WasmEdgeZlibInflateBackInit_::body(
    const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr, int32_t WindowBits,
    uint32_t WindowPtr, uint32_t VersionPtr, int32_t StreamSize) {
  if (!CheckSize(StreamSize))
    return static_cast<int32_t>(Z_VERSION_ERROR);

  MEMINST_CHECK(MemInst, Frame, 0)

  const auto WasmZlibVersion =
      getBoundedInBoundsCString(*MemInst, VersionPtr, MaxZlibVersionLen);
  if (!WasmZlibVersion || !CheckVersion(WasmZlibVersion->data())) {
    return static_cast<int32_t>(Z_VERSION_ERROR);
  }
  uint8_t *Window = nullptr;
  if (WindowPtr != 0 && WindowBits >= 8 && WindowBits <= 15) {
    const auto WindowSpan =
        MemInst->getSpan<uint8_t>(WindowPtr, UINT64_C(1) << WindowBits);
    if (unlikely(WindowSpan.data() == nullptr)) {
      spdlog::error("[WasmEdge-Zlib] [InflateBackInit] "sv
                    "Out-of-bounds window buffer."sv);
      return Unexpect(ErrCode::Value::HostFuncError);
    }
    Window = WindowSpan.data();
  } else {
    Window = nullptr;
  }

  return initStream("WasmEdgeZlibInflateBackInit_", Env, ZStreamPtr, Frame,
                    ZStreamKind::InflateBack, [&](z_stream *HostZStream) {
                      return inflateBackInit_(HostZStream, WindowBits, Window,
                                              WasmZlibVersion->data(),
                                              sizeof(z_stream));
                    });
}

Expect<int32_t> WasmEdgeZlibGZGetc_::body(const Runtime::CallingFrame &,
                                          uint32_t GZFile) {
  const auto GZFileIt = Env.GZFileMap.find(GZFile);
  if (GZFileIt == Env.GZFileMap.end()) {
    spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibGZGetc_] "sv
                  "Invalid GZFile received."sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return gzgetc_(GZFileIt->second.GZ);
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
  if (HostZStreamIt->second.Kind != ZStreamKind::Inflate) {
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }

  return inflateSyncPoint(&HostZStreamIt->second.Z);
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
  if (HostZStreamIt->second.Kind != ZStreamKind::Inflate) {
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }

  return inflateUndermine(&HostZStreamIt->second.Z, Subvert);
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
  if (HostZStreamIt->second.Kind != ZStreamKind::Inflate) {
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }

  return inflateValidate(&HostZStreamIt->second.Z, Check);
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
  if (HostZStreamIt->second.Kind != ZStreamKind::Inflate) {
    return INT32_C(-1);
  }

  return inflateCodesUsed(&HostZStreamIt->second.Z);
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
  if (HostZStreamIt->second.Kind != ZStreamKind::Inflate) {
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }

  const auto ZRes = inflateResetKeep(&HostZStreamIt->second.Z);

  if (ZRes == Z_OK) {
    Env.GZHeaderMap.erase(ZStreamPtr);
    HostZStreamIt->second.MayNeedDict = false;
  }

  return ZRes;
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
  if (HostZStreamIt->second.Kind != ZStreamKind::Deflate) {
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }

  const int ZRes = deflateResetKeep(&HostZStreamIt->second.Z);

  if (ZRes == Z_OK) {
    HostZStreamIt->second.DeflateStarted = false;
  }

  return ZRes;
}

} // namespace Host
} // namespace WasmEdge
