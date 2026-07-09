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
  auto HostZStream = HostZStreamIt->second.get();
  const auto GZHeaderStoreIt = Env.GZHeaderMap.find(ZStreamPtr);

  const auto InSpan = MemInst->getSpan<unsigned char>(ModuleZStream->NextIn,
                                                      ModuleZStream->AvailIn);
  const auto OutSpan = MemInst->getSpan<unsigned char>(ModuleZStream->NextOut,
                                                       ModuleZStream->AvailOut);
  // Only the streaming callbacks read next_in or write next_out, so only they
  // must reject an out-of-bounds data buffer (inflateSync scans input but
  // never writes output). Control operations (init, reset, end, copy, ...)
  // ignore these fields, and zlib's contract lets a guest leave them
  // uninitialized; trapping them would break valid init and cleanup calls.
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

  // Only inflate() syncs the header, and only the guest-owned INPUT fields:
  // the buffer locations (extra/name/comment) and their *_max capacities,
  // re-validated against linear memory every call; a guest null (0) location
  // is zlib's Z_NULL ("field not requested") and must not resolve to wasm
  // address 0. The OUTPUT fields (text/time/xflags/os/extra_len/hcrc/done)
  // stay zlib-owned and flow guest-ward only in the write-back below;
  // re-injecting extra_len would let a guest drive the EXTRA-state write
  // offset out of bounds. Cleanup/reset calls skip this (SyncGZHeader=false),
  // so a corrupted *_max never aborts a stream teardown.
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
    // zlib never writes through a zero-capacity field yet still distinguishes
    // the caller's non-null pointer from Z_NULL ("field not requested"), so a
    // stale zero-capacity guest pointer past linear memory must stay non-null
    // rather than read as an absent field; the shared dummy is never written
    // (every header store is bounded by its *_max), and the delta write-back
    // below leaves the guest offset untouched for it.
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

      // zlib sets an absent optional field's pointer to Z_NULL "to signal its
      // absence" even when a buffer was supplied; surface that as guest null
      // instead of subtracting a live pointer from null.
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

  // deflate() refuses a flush outside [Z_NO_FLUSH, Z_BLOCK] before touching
  // next_in or next_out, so as with deflateParams validate the buffers only
  // for a flush value zlib will service.
  const bool FlushValid = Flush >= Z_NO_FLUSH && Flush <= Z_BLOCK;
  return SyncRun(
      "WasmEdgeZlibDeflate", Env, ZStreamPtr, Frame,
      [&](z_stream *HostZStream) { return deflate(HostZStream, Flush); },
      /*ValidateInputBuffer=*/FlushValid, /*ValidateOutputBuffer=*/FlushValid);
}

Expect<int32_t> WasmEdgeZlibDeflateEnd::body(const Runtime::CallingFrame &Frame,
                                             uint32_t ZStreamPtr) {

  const auto ZRes =
      SyncRun("WasmEdgeZlibDeflateEnd", Env, ZStreamPtr, Frame,
              [&](z_stream *HostZStream) { return deflateEnd(HostZStream); });

  // deflateEnd frees zlib's internal state on both Z_OK and Z_DATA_ERROR (the
  // latter when the stream is ended mid-compression), so drop the host tracking
  // on either result. Only Z_STREAM_ERROR (an already-invalid stream) leaves
  // it.
  if (ZRes == Z_OK || ZRes == Z_DATA_ERROR) {
    Env.ZStreamMap.erase(ZStreamPtr);
    // Drop the header snapshot; zlib no longer references it after deflateEnd.
    // A deflateCopy'd stream that shares it keeps it alive via the shared_ptr.
    Env.GZHeaderMap.erase(ZStreamPtr);
  }

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
      [&](z_stream *HostZStream) { return inflate(HostZStream, Flush); },
      /*ValidateInputBuffer=*/true, /*ValidateOutputBuffer=*/true,
      /*SyncGZHeader=*/true);
}

Expect<int32_t> WasmEdgeZlibInflateEnd::body(const Runtime::CallingFrame &Frame,
                                             uint32_t ZStreamPtr) {

  const auto ZRes =
      SyncRun("WasmEdgeZlibInflateEnd", Env, ZStreamPtr, Frame,
              [&](z_stream *HostZStream) { return inflateEnd(HostZStream); });

  Env.ZStreamMap.erase(ZStreamPtr);
  // Drop the header snapshot captured by inflateGetHeader; zlib no longer
  // references it after inflateEnd.
  Env.GZHeaderMap.erase(ZStreamPtr);

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

  if (ZRes != Z_OK) {
    if (Inserted)
      Env.ZStreamMap.erase(It);
    return ZRes;
  }

  // deflateCopy duplicated the source's internal gz_header pointer into the
  // destination stream. Share the same reference-counted header snapshot so it
  // stays alive for the copy even after the source later replaces or ends its
  // own header; the deflate snapshot is immutable, so sharing is safe.
  const auto SourceHeaderIt = Env.GZHeaderMap.find(SourcePtr);
  if (SourceHeaderIt != Env.GZHeaderMap.end())
    Env.GZHeaderMap.insert_or_assign(DestPtr, SourceHeaderIt->second);

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

  // deflateParams rejects an out-of-range level or strategy before consuming
  // next_in or producing into next_out, so buffers only need to prove
  // themselves for a call zlib will actually service; a rejected call still
  // gets zlib's Z_STREAM_ERROR, and an unresolved span stays untouched
  // because deflate() re-checks null buffers itself.
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

  MEMINST_CHECK(MemInst, Frame, 0)

  // Native deflateSetHeader accepts Z_NULL and reverts the stream to the
  // default gzip header; guest null (0) takes that path rather than
  // dereferencing wasm address 0. zlib then retains no pointer into any
  // snapshot, so a published one can be dropped.
  if (HeadPtr == 0) {
    const auto ZRes = SyncRun("WasmEdgeZlibDeflateSetHeader", Env, ZStreamPtr,
                              Frame, [&](z_stream *HostZStream) {
                                return deflateSetHeader(HostZStream, Z_NULL);
                              });
    if (ZRes && *ZRes == Z_OK)
      Env.GZHeaderMap.erase(ZStreamPtr);
    return ZRes;
  }

  // Get zlib's verdict before any guest-header work: only a gzip deflate
  // stream accepts a header, and deflateSetHeader merely validates the stream
  // and stores the pointer, so re-storing the currently published snapshot
  // (or Z_NULL when none was published) changes nothing. A stream zlib
  // rejects thus fails with its cheap Z_STREAM_ERROR before the host reads
  // the header or snapshots a guest-sized extra/name/comment into host
  // memory, and an invalid ZStreamPtr still traps inside SyncRun.
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

  // Snapshot the header into host-owned storage; deflate() emits the name and
  // comment incrementally across calls (resuming at an internal index), so the
  // buffers must stay stable and must not be re-read from mutable guest memory.
  // The store is heap-allocated and reference-counted so deflateCopy can hand
  // the same snapshot to a copied stream; its address stays stable, so the
  // bound buffer pointers remain valid once it is inserted into the map.
  auto Store = std::make_shared<WasmEdgeZlibEnvironment::GZStore>();
  Store->WasmGZHeaderOffset = HeadPtr;
  Store->IsInflate = false;
  Store->HostGZHeader = std::make_unique<gz_header>();

  const bool HasExtra =
      ModuleGZHeader->Extra != 0 && ModuleGZHeader->ExtraLen != 0;
  const bool HasName = ModuleGZHeader->Name != 0;
  const bool HasComment = ModuleGZHeader->Comment != 0;

  if (HasExtra) {
    const auto ExtraSpan = MemInst->getSpan<Bytef>(ModuleGZHeader->Extra,
                                                   ModuleGZHeader->ExtraLen);
    if (unlikely(ExtraSpan.data() == nullptr)) {
      spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibDeflateSetHeader] "sv
                    "Out-of-bounds gzip header extra field."sv);
      return Unexpect(ErrCode::Value::HostFuncError);
    }
    Store->Extra.assign(ExtraSpan.begin(), ExtraSpan.end());
  }
  if (HasName) {
    const auto *NameStr = getInBoundsCString(*MemInst, ModuleGZHeader->Name);
    if (unlikely(NameStr == nullptr)) {
      spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibDeflateSetHeader] "sv
                    "Out-of-bounds gzip header name field."sv);
      return Unexpect(ErrCode::Value::HostFuncError);
    }
    Store->Name = NameStr;
  }
  if (HasComment) {
    const auto *CommentStr =
        getInBoundsCString(*MemInst, ModuleGZHeader->Comment);
    if (unlikely(CommentStr == nullptr)) {
      spdlog::error("[WasmEdge-Zlib] [WasmEdgeZlibDeflateSetHeader] "sv
                    "Out-of-bounds gzip header comment field."sv);
      return Unexpect(ErrCode::Value::HostFuncError);
    }
    Store->Comment = CommentStr;
  }

  Store->HostGZHeader->text = ModuleGZHeader->Text;
  Store->HostGZHeader->time = ModuleGZHeader->Time;
  Store->HostGZHeader->xflags = ModuleGZHeader->XFlags;
  Store->HostGZHeader->os = ModuleGZHeader->OS;
  Store->HostGZHeader->hcrc = ModuleGZHeader->HCRC;
  Store->HostGZHeader->extra_len = ModuleGZHeader->ExtraLen;

  // Point the gz_header at the store's own snapshot buffers. The store keeps a
  // stable heap address, so this binding stays valid after it is inserted.
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

  // Publish the snapshot only after zlib accepts it. On any failure the
  // previously stored header stays in place, since zlib (or a deflateCopy'd
  // stream) may still hold a pointer into it from an earlier success.
  if (ZRes == Z_OK)
    Env.GZHeaderMap.insert_or_assign(ZStreamPtr, std::move(Store));

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

  if (ZRes != Z_OK) {
    if (Inserted)
      Env.ZStreamMap.erase(It);
    return ZRes;
  }

  // inflateCopy duplicated the source's internal gz_header pointer into the
  // destination stream. Share the same reference-counted header snapshot so it
  // stays alive for the copy even after the source later replaces or ends its
  // own header; zlib's copied head aliases this exact host storage.
  const auto SourceHeaderIt = Env.GZHeaderMap.find(SourcePtr);
  if (SourceHeaderIt != Env.GZHeaderMap.end())
    Env.GZHeaderMap.insert_or_assign(DestPtr, SourceHeaderIt->second);

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibInflateReset::body(const Runtime::CallingFrame &Frame,
                               uint32_t ZStreamPtr) {

  const auto ZRes =
      SyncRun("WasmEdgeZlibInflateReset", Env, ZStreamPtr, Frame,
              [&](z_stream *HostZStream) { return inflateReset(HostZStream); });

  // A successful inflate reset detaches zlib's internal gzip-header pointer
  // (the guest must call inflateGetHeader again for the next stream), so the
  // snapshot must be dropped with it: a stale entry would keep re-validating
  // and rewriting a guest header zlib no longer references, failing legitimate
  // post-reset inflate calls once the guest reuses that memory. deflate resets
  // keep zlib's gzhead, so deflate snapshots must stay alive.
  if (ZRes && *ZRes == Z_OK)
    Env.GZHeaderMap.erase(ZStreamPtr);

  return ZRes;
}

Expect<int32_t>
WasmEdgeZlibInflateReset2::body(const Runtime::CallingFrame &Frame,
                                uint32_t ZStreamPtr, int32_t WindowBits) {

  const auto ZRes = SyncRun("WasmEdgeZlibInflateReset2", Env, ZStreamPtr, Frame,
                            [&](z_stream *HostZStream) {
                              return inflateReset2(HostZStream, WindowBits);
                            });

  // inflateReset2 detaches zlib's gzip-header pointer like inflateReset; drop
  // the stale snapshot (see WasmEdgeZlibInflateReset).
  if (ZRes && *ZRes == Z_OK)
    Env.GZHeaderMap.erase(ZStreamPtr);

  return ZRes;
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

  MEMINST_CHECK(MemInst, Frame, 0)

  // Validate the guest header pointer up front (like deflateSetHeader) so an
  // out-of-bounds HeadPtr is rejected here instead of being stored and then
  // failing every later inflate/cleanup call once zlib has accepted the stream.
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

  // Publish the snapshot only after zlib accepts it and points its internal
  // head at it. A repeated call replaces the previous snapshot, which zlib no
  // longer references; on failure zlib left its head untouched, so the local
  // store is simply dropped and any previously stored header stays in place.
  if (ZRes == Z_OK)
    Env.GZHeaderMap.insert_or_assign(ZStreamPtr, std::move(Store));

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

  // Z_STREAM_ERROR means the handle was the wrong mode and was NOT freed; keep
  // it so the environment destructor can still reclaim it.
  if (ZRes != Z_STREAM_ERROR)
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

  // Z_STREAM_ERROR means the handle was the wrong mode and was NOT freed; keep
  // it so the environment destructor can still reclaim it.
  if (ZRes != Z_STREAM_ERROR)
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

  const auto ZRes = inflateResetKeep(HostZStreamIt->second.get());

  // inflateResetKeep detaches zlib's gzip-header pointer like inflateReset;
  // drop the stale snapshot (see WasmEdgeZlibInflateReset).
  if (ZRes == Z_OK)
    Env.GZHeaderMap.erase(ZStreamPtr);

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

  return deflateResetKeep(HostZStreamIt->second.get());
}

} // namespace Host
} // namespace WasmEdge
