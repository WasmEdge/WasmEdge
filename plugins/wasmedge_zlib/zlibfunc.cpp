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

// zlib's versioned init entry points reject a null or major-mismatched version
// before they examine any other argument.
static inline bool CheckVersion(const char *Version) noexcept {
  return Version != nullptr && Version[0] == ZLIB_VERSION[0];
}

constexpr bool CheckSize(int32_t StreamSize) {

  return (StreamSize == static_cast<int32_t>(sizeof(WasmZStream)));
}

constexpr uint64_t MaxGZHeaderStringLen = UINT64_C(1) << 20;

// zlib's versioned initializers inspect only version[0], so cap the version
// scan well above any real ZLIB_VERSION instead of letting a guest pointer
// into a large non-NUL region drive an unbounded memchr over linear memory.
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

  // deflateCopy/inflateCopy overwrite the host stream with the source's
  // retained next_in/next_out, so copying onto a destination whose own buffer
  // did not resolve can leave one side of this delta null while the other is a
  // live pointer. Subtracting a null from a non-null pointer is undefined, so
  // advance the guest offset only when both ends are real; every other callback
  // keeps both ends in the same buffer, where the guard always passes.
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

// Create a fresh tracked stream, run zlib's init on it, and reject re-init over
// a live key. Running an init on an already-initialized z_stream would let zlib
// overwrite strm->state and leak the previous internal state (unbounded, since
// nothing else frees it). On a failed init the (empty) entry is dropped so the
// guest may retry.
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

  // An errored Expect compares unequal to nothing (both == and != yield
  // false), so test it explicitly: a SyncRun trap must also drop the entry,
  // or the failed key would be treated as live forever.
  if (!ZRes || *ZRes != Z_OK)
    Env.ZStreamMap.erase(It);

  return ZRes;
}

// zlib's stream functions verify the stream kind first (deflateStateCheck /
// inflateStateCheck) and answer a mismatched stream with Z_STREAM_ERROR
// before reading or writing anything else. Classic zlib decides that
// deterministically for a deflate<->inflate mix (the punned status and mode
// value ranges are disjoint), but zlib-ng's layouts differ enough that the
// same probe reads arbitrary fields: a cross-kind call may pass its checks
// and then free garbage pointers (deflateEnd), overwrite live internal state
// (inflateReset/inflatePrime/inflateValidate), or copy past the allocation
// (inflateCopy) -- and inflateBackEnd frees whatever state it is handed
// without any check at all. Answer from the tracked kind instead of entering
// zlib with a mismatched state; an untracked ZStreamPtr keeps its usual trap.
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

  // deflate() refuses a flush outside [Z_NO_FLUSH, Z_BLOCK] before touching
  // next_in or next_out, so as with deflateParams validate the buffers only
  // for a flush value zlib will service.
  const bool FlushValid = Flush >= Z_NO_FLUSH && Flush <= Z_BLOCK;
  const auto ZRes = SyncRun(
      "WasmEdgeZlibDeflate", Env, ZStreamPtr, Frame,
      [&](z_stream *HostZStream) { return deflate(HostZStream, Flush); },
      /*ValidateInputBuffer=*/FlushValid, /*ValidateOutputBuffer=*/FlushValid);

  // A deflate() that returns Z_OK/Z_STREAM_END has written the header and left
  // INIT_STATE, the state a later deflateSetDictionary refuses; record it so
  // that guard can answer without validating the dictionary span.
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

  // deflateEnd frees zlib's internal state on both Z_OK and Z_DATA_ERROR (the
  // latter when the stream is ended mid-compression), so drop the host tracking
  // on either result. Only Z_STREAM_ERROR (an already-invalid stream) leaves
  // it, as does a SyncRun trap (zlib never ran, so nothing was freed).
  if (ZRes && (*ZRes == Z_OK || *ZRes == Z_DATA_ERROR)) {
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

  // Z_NEED_DICT is the only way into DICT mode, and only a later inflate()
  // leaves it (a successful inflateSetDictionary keeps the mode until then),
  // so inflate results alone track whether zlib may be awaiting a dictionary.
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

  // Z_STREAM_ERROR means zlib did not free the stream (a wrong-type or
  // already-invalid stream); keep the entry so its internal state is not leaked
  // and the correct *End can still reclaim it. A SyncRun trap keeps it for the
  // same reason. Any other result freed zlib's state, so drop the tracking and
  // the header snapshot with it.
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

  // deflateSetDictionary reads the dictionary immediately except when it
  // rejects the request first: a wrong-kind stream fails the state check, a
  // gzip-wrapped deflate stream refuses any preset dictionary, and a
  // zlib-wrapped stream that has already produced output (left INIT_STATE)
  // refuses one too, all before touching the bytes -- answer those from the
  // tracked state. A raw deflate stream instead rejects on live lookahead,
  // which is not tracked here, so that case still validates the buffer.
  if (const auto It = Env.ZStreamMap.find(ZStreamPtr);
      It != Env.ZStreamMap.end() &&
      (It->second.Kind != ZStreamKind::Deflate || It->second.GzipWrap ||
       (!It->second.RawDeflate && It->second.DeflateStarted))) {
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }

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
  if (ZStreamIt->second.Kind != ZStreamKind::Deflate) {
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }

  // zlib writes up to the internal window (~32 KiB) with no caller-supplied
  // cap, so query the exact length first and validate the destination for it.
  uInt NeededLen = 0;
  deflateGetDictionary(&ZStreamIt->second.Z, Z_NULL, &NeededLen);

  // Both out-pointers are optional: a Z_NULL dictionary returns only the
  // length and a Z_NULL dictLength is not set, so a guest null (0) maps to
  // Z_NULL rather than to wasm address 0.
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
  // Capture the source stream pointer before try_emplace may rehash the map;
  // node addresses are stable across rehash, iterators are not.
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

  // Reject copying onto a live destination (this also rejects a self-copy where
  // DestPtr == SourcePtr): deflateCopy would overwrite the destination stream,
  // leaking its state, and on an allocation failure leave the destination
  // aliasing the source's internal state.
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

  // A SyncRun trap (errored Expect) compares unequal to nothing, so test it
  // explicitly: the failed destination entry must be dropped, and the header
  // sharing below is for a successful copy only.
  if (!ZRes || *ZRes != Z_OK) {
    // deflateCopy may have left the destination aliasing the source's internal
    // state; detach it so erasing the entry does not free the source's state.
    It->second.Z.state = Z_NULL;
    Env.ZStreamMap.erase(It);
    return ZRes;
  }

  // The copy inherits the source's wrapper and progress: zlib duplicated the
  // internal state wholesale, wrap and status included, so the dictionary
  // rejects keep answering for the copy exactly as they would for the source.
  It->second.GzipWrap = SourceGzipWrap;
  It->second.RawDeflate = SourceRawDeflate;
  It->second.DeflateStarted = SourceDeflateStarted;

  // deflateCopy duplicated the source's internal gz_header pointer into the
  // destination stream. Share the same reference-counted header snapshot so it
  // stays alive for the copy even after the source later replaces or ends its
  // own header; the deflate snapshot is immutable, so sharing is safe.
  const auto SourceHeaderIt = Env.GZHeaderMap.find(SourcePtr);
  if (SourceHeaderIt != Env.GZHeaderMap.end())
    Env.GZHeaderMap.insert_or_assign(DestPtr, SourceHeaderIt->second);

  // zlib overwrote the destination host stream with the source wholesale, but
  // the delta write-back cannot express that against an uninitialized (legal
  // here) destination's pre-call pointers. Copy the stream fields directly
  // from the source's guest-visible struct instead.
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

  // deflateReset returns the stream to INIT_STATE, so a preset dictionary is
  // allowed again until the next deflate().
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

  // No kind guard here: deflateBound only reads scalar fields that stay
  // inside any tracked state's allocation and falls back to its conservative
  // bound when the checks fail, so a wrong-kind stream is safe host-side and
  // gets the same answer a native caller would.
  return SyncRun("WasmEdgeZlibDeflateBound", Env, ZStreamPtr, Frame,
                 [&](z_stream *HostZStream) {
                   return deflateBound(HostZStream, SourceLen);
                 });
}

Expect<int32_t>
WasmEdgeZlibDeflatePending::body(const Runtime::CallingFrame &Frame,
                                 uint32_t ZStreamPtr, uint32_t PendingPtr,
                                 uint32_t BitsPtr) {

  // A wrong-kind stream is refused before zlib writes either out-parameter,
  // so the guest pointers are validated only for a call zlib will service.
  if (streamKindMismatch(Env, ZStreamPtr, ZStreamKind::Deflate)) {
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }

  MEMINST_CHECK(MemInst, Frame, 0)

  // Both out-pointers are optional: zlib does not set a Z_NULL pending or
  // bits, so a guest null (0) maps to Z_NULL rather than to wasm address 0.
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

  // The gzip extra field length is 16-bit: zlib writes only the low 16 bits of
  // extra_len as XLEN and emits at most that many bytes. Snapshot (and bounds-
  // check) only what zlib will use, so a guest-declared multi-GB ExtraLen can
  // neither force a matching host allocation nor spuriously fail the bounds
  // check for bytes zlib never reads.
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
      // A non-null extra with a zero emitted length still makes zlib set the
      // gzip FEXTRA flag with XLEN=0. Keep a one-byte placeholder so the
      // published pointer stays non-null (an empty vector's data() may be
      // null); extra_len is 0, so zlib never reads the byte.
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

  // Publish the snapshot only after zlib accepts it. On any failure (a zlib
  // error or a SyncRun trap) the previously stored header stays in place,
  // since zlib (or a deflateCopy'd stream) may still hold a pointer into it
  // from an earlier success.
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
  // wrap is fixed by windowBits: negative selects a raw stream and 16+
  // enables the gzip wrapper (24..31 gzip-only, 40..47 auto-detect).
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

  // inflateSetDictionary reads the dictionary only when the stream can accept
  // it: raw inflate folds it into the window at once and DICT mode checksums
  // it, but a wrapped stream that is not waiting for a dictionary (and any
  // non-inflate stream) answers Z_STREAM_ERROR before touching the bytes, so
  // only the reading cases validate the guest buffer.
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

  // zlib writes up to the internal window (~32 KiB) with no caller-supplied
  // cap, so query the exact length first and validate the destination for it.
  uInt NeededLen = 0;
  inflateGetDictionary(&ZStreamIt->second.Z, Z_NULL, &NeededLen);

  // Both out-pointers are optional: a Z_NULL dictionary returns only the
  // length and a Z_NULL dictLength is not set, so a guest null (0) maps to
  // Z_NULL rather than to wasm address 0.
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
  // Capture the source stream pointer before try_emplace may rehash the map;
  // node addresses are stable across rehash, iterators are not.
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

  // Reject copying onto a live destination (this also rejects a self-copy where
  // DestPtr == SourcePtr): inflateCopy would overwrite the destination stream,
  // leaking its state.
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

  // A SyncRun trap (errored Expect) compares unequal to nothing, so test it
  // explicitly: the failed destination entry must be dropped, and the header
  // sharing below is for a successful copy only.
  if (!ZRes || *ZRes != Z_OK) {
    // inflateCopy allocates before touching the destination, so a failure
    // leaves its state null; detach defensively before erasing regardless.
    It->second.Z.state = Z_NULL;
    Env.ZStreamMap.erase(It);
    return ZRes;
  }

  // The copy inherits the source's wrapper and dictionary-wait state: zlib
  // duplicated the internal state wholesale, wrap and mode included.
  It->second.GzipWrap = SourceGzipWrap;
  It->second.RawInflate = SourceRawInflate;
  It->second.MayNeedDict = SourceMayNeedDict;

  // inflateCopy duplicated the source's internal gz_header pointer into the
  // destination stream. Share the same reference-counted header snapshot so it
  // stays alive for the copy even after the source later replaces or ends its
  // own header; zlib's copied head aliases this exact host storage.
  const auto SourceHeaderIt = Env.GZHeaderMap.find(SourcePtr);
  if (SourceHeaderIt != Env.GZHeaderMap.end())
    Env.GZHeaderMap.insert_or_assign(DestPtr, SourceHeaderIt->second);

  // Same as deflateCopy: express the copied stream fields directly from the
  // source's guest-visible struct instead of the delta write-back, which
  // cannot resolve an uninitialized destination's offsets.
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

  // A successful inflate reset detaches zlib's internal gzip-header pointer
  // (the guest must call inflateGetHeader again for the next stream), so the
  // snapshot must be dropped with it: a stale entry would keep re-validating
  // and rewriting a guest header zlib no longer references, failing legitimate
  // post-reset inflate calls once the guest reuses that memory. deflate resets
  // keep zlib's gzhead, so deflate snapshots must stay alive.
  if (ZRes && *ZRes == Z_OK) {
    Env.GZHeaderMap.erase(ZStreamPtr);
    // The stream is back at the header stage, no longer awaiting a dictionary.
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

  // inflateReset2 detaches zlib's gzip-header pointer like inflateReset; drop
  // the stale snapshot (see WasmEdgeZlibInflateReset).
  if (ZRes && *ZRes == Z_OK) {
    Env.GZHeaderMap.erase(ZStreamPtr);
    // inflateReset2 re-derives wrap from the new windowBits and restarts at
    // the header stage (see WasmEdgeZlibInflateInit2 for the mapping).
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

  // inflateMark's bad-state answer is -(1 << 16), not Z_STREAM_ERROR.
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

  // inflateGetHeader stores the header and writes head->done only on a
  // gzip-capable inflate stream (wrap & 2); every other stream gets
  // Z_STREAM_ERROR before the header is touched, so the guest pointer is
  // validated only when zlib will actually accept it. wrap is fixed by
  // windowBits at init/reset2 time, so the tracked flag mirrors it exactly.
  if (const auto It = Env.ZStreamMap.find(ZStreamPtr);
      It != Env.ZStreamMap.end() &&
      (It->second.Kind != ZStreamKind::Inflate || !It->second.GzipWrap)) {
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }

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
  // longer references; on failure (a zlib error or a SyncRun trap) zlib left
  // its head untouched, so the local store is simply dropped and any
  // previously stored header stays in place.
  if (ZRes && *ZRes == Z_OK) {
    // inflateGetHeader reset head->done to 0 as it registered the header.
    // Deliver that now: the store was not yet published during this SyncRun,
    // so the next inflate would otherwise be the first call to sync it back.
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
    // A guest null window is zlib's Z_NULL and an out-of-range windowBits is
    // equally rejected before the window is touched, so neither resolves a
    // guest pointer (wasm address 0 must not stand in for Z_NULL here).
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

  // inflateBackEnd frees whatever state it is handed -- it never checks the
  // kind -- so without this a guest could free a live deflate/inflate state
  // through it and leak that stream's separately allocated internals.
  if (streamKindMismatch(Env, ZStreamPtr, ZStreamKind::InflateBack)) {
    return static_cast<int32_t>(Z_STREAM_ERROR);
  }

  const auto ZRes = SyncRun(
      "WasmEdgeZlibInflateBackEnd", Env, ZStreamPtr, Frame,
      [&](z_stream *HostZStream) { return inflateBackEnd(HostZStream); });

  // Keep the entry on Z_STREAM_ERROR or a SyncRun trap (zlib did not free it);
  // otherwise drop it.
  if (ZRes && *ZRes != Z_STREAM_ERROR)
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

// Return the WASI environment backing this instance, or nullptr when the
// embedder did not configure WASI. FStream and the wasi_nn plugins take the
// same const_cast to reach WASI's mutable fd table from a host function's const
// view of the environment.
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

// Release the guest WASI fd a gz handle owns (see GZFileEntry::OwnedWasiFd)
// once zlib has freed the handle. A close that fails because the guest already
// released the fd itself is ignored: that guest hits the same double close
// natively.
static void releaseOwnedWasiFd(const Runtime::CallingFrame &Frame,
                               int64_t OwnedWasiFd) noexcept {
  if (OwnedWasiFd < 0) {
    return;
  }
  if (auto *WasiEnv = getWasiEnviron(Frame); WasiEnv != nullptr) {
    WasiEnv->fdClose(static_cast<__wasi_fd_t>(OwnedWasiFd));
  }
}

// Duplicate a WASI-owned host descriptor so zlib owns an independent one
// (gzclose closes it) without disturbing WASI's own descriptor. Returns -1 on
// failure.
#if defined(_WIN32)
// Bridging a WASI descriptor to a zlib gzFile on Windows needs the underlying
// HANDLE duplicated (getNativeHandler yields a HANDLE, not a CRT fd). Until the
// winapi wrapper exposes DuplicateHandle, report open failure here rather than
// pull in <windows.h> or risk a double close; the guest sees a null handle.
// TODO: Bridge the HANDLE to a CRT fd (DuplicateHandle + _open_osfhandle) once
// the winapi wrapper exposes it, and add Windows CI coverage for this plugin so
// the bridge is exercised.
static int dupNativeHandle(uint64_t) noexcept { return -1; }
static void closeNativeFd(int) noexcept {}
static int64_t tellNativeFd(int) noexcept { return -1; }
static void seekNativeFd(int, int64_t) noexcept {}
#else
static int dupNativeHandle(uint64_t Native) noexcept {
  // F_DUPFD_CLOEXEC rather than a bare dup(): WASI opens every descriptor with
  // O_CLOEXEC, so zlib's duplicate must keep close-on-exec too. A bare dup()
  // clears it, leaking the sandboxed fd across an execve while the gz handle is
  // live, and gzdopen cannot reapply zlib's "e" mode to an already-open fd.
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

// The subset of zlib's gz_open mode parse that decides how the file is
// opened: the last of r/w/a wins, 'x' requests an exclusive create, 'N'
// (zlib >= 1.3.2) requests a nonblocking open, and the last of 'T'/'G'
// (the latter zlib >= 1.3.2) picks between a transparent and a forced-gzip
// stream. Any '+', a mode with no direction at all, a transparent read, or
// a forced-gzip write or append is rejected -- gz_open refuses those before
// opening the file. Earlier zlib ignores 'G' and 'N' outright, so refusing
// here is the fail-closed reading that keeps a refused mode free of pathOpen
// side effects on every zlib. Every other character only affects stream
// semantics, not the open itself.
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

// Resolve a WASI fd to its host descriptor (capability-checked by the WASI fd
// table), duplicate it, and hand the duplicate to zlib. When OwnWasiFd is set
// the WASI fd was opened for this call and is released here; otherwise it
// belongs to the guest and is left intact here -- the gzdopen caller records
// it on the handle so the gzclose that frees the handle releases it. A failure
// returns a null-handle entry.
static WasmEdgeZlibEnvironment::GZFileEntry
wasiFdToGZ(WASI::Environ &WasiEnv, __wasi_fd_t WasiFd, const char *Mode,
           bool OwnWasiFd) noexcept {
  // Enforce the WASI rights that mediate this fd before handing zlib a raw
  // duplicate it operates on outside WASI's checks. Only the access direction
  // is required up front: FD_SEEK/FD_TELL stay optional so an unseekable
  // stream (a pipe or stdout) still opens, and are recorded on the handle for
  // the gz calls that would lseek the duplicate. Append mode is the one
  // exception -- gz_open itself lseeks the descriptor to EOF at open time --
  // so it requires FD_SEEK unless the fd is already append-only, where writes
  // land at EOF regardless and the open-time reposition is undone below.
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
    // On Windows this is the expected fail-closed path (dupNativeHandle cannot
    // bridge the HANDLE to a CRT fd yet; see its TODO); elsewhere it means the
    // host ran out of descriptors. Either way the guest sees a null handle.
    spdlog::error("[WasmEdge-Zlib] [wasiFdToGZ] "sv
                  "Failed to duplicate the host descriptor for zlib."sv);
    return {};
  }
  // gz_open lseeks an append-mode descriptor to EOF at open time, and the
  // duplicate shares the guest fd's file offset. When the fd was admitted
  // through the append-only exception (no FD_SEEK), that reposition would
  // still be observable through the fd's remaining FD_TELL/FD_READ rights, so
  // put the shared offset back; the append flag keeps writes landing at EOF
  // regardless of the offset.
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
    // gzdopen does not close the descriptor on failure.
    closeNativeFd(DupFd);
    return {};
  }
  return {GZ, CanSeek, CanTell, RestoredAppendOffset,
          Parsed->Dir == GZOpenDir::Read};
}

// Open a guest path through the WASI capability layer (relative to preopen fd
// 3, the default working directory) so the guest can only reach files WASI
// granted, then bridge the host descriptor to zlib. Guest paths are
// normalized the way wasi-libc's open() starts its preopen match: leading '/'
// and './' runs are stripped (the libc default cwd is '/'), so an absolute
// spelling reaches the same sandbox entry as its relative one. With several
// preopens only fd 3 is consulted -- a documented envelope until a full
// preopen-table match exists. The mode is parsed the way zlib's own gz_open
// does, and before pathOpen runs: gzopen must fail without touching the file
// on a mode zlib rejects (such as "w+"), and an exclusive "wx" create must
// refuse to clobber an existing sandbox file rather than truncate it. zlib's
// own gz_open re-parses the same string afterwards and stays the final
// authority on the stream semantics.
static WasmEdgeZlibEnvironment::GZFileEntry
wasiGZOpen(WASI::Environ &WasiEnv, const char *Path,
           const char *Mode) noexcept {
  const auto Parsed = parseGZOpenMode(Mode);
  if (!Parsed) {
    return {};
  }
#if defined(_WIN32)
  // dupNativeHandle cannot bridge a WASI handle to a CRT descriptor yet, so
  // this open always fails; fail before pathOpen can create or truncate the
  // file as a side effect. This is an expected Windows limitation (see the
  // dupNativeHandle TODO), surfaced to the guest as a null handle.
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
  // zlib >= 1.3.2 puts 'N' into the open(2) flags themselves; a FIFO open
  // blocks (or fails with ENXIO on the write side) before gzdopen could apply
  // the flag to the descriptor, so the request must ride pathOpen.
  if (Parsed->NonBlock) {
    FdFlags = static_cast<__wasi_fdflags_t>(FdFlags | __WASI_FDFLAGS_NONBLOCK);
  }
  // FD_SEEK/FD_TELL are optional for streaming use: wasiFdToGZ records them on
  // the handle and the gz calls that would reposition the descriptor enforce
  // them. Request them only when the preopen can inherit them (FD_SEEK implies
  // FD_TELL, matching the fd table's own rights check); requiring them outright
  // would make pathOpen reject a plain streaming gzopen on a directory narrowed
  // to its direction rights.
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
  // Native gzopen follows a final symlink the way wasi-libc's open() does
  // (LOOKUP_SYMLINK_FOLLOW by default), and resolvePath re-walks the target
  // with the same preopen-escape checks a guest path_open gets, so follow
  // here too. The exception is an exclusive create: native O_EXCL refuses an
  // existing symlink -- dangling included -- rather than following it, and
  // gzopen applies O_EXCL only when creating (write or append); an exclusive
  // read mode such as "rx" opens plainly.
  auto LookupFlags = __WASI_LOOKUPFLAGS_SYMLINK_FOLLOW;
  if (Parsed->Exclusive && Parsed->Dir != GZOpenDir::Read) {
    OpenFlags |= __WASI_OFLAGS_EXCL;
    LookupFlags = static_cast<__wasi_lookupflags_t>(0);
  }
  // resolvePath refuses an absolute path outright, so strip the leading '/'
  // and './' runs the way wasi-libc does before it matches a preopen; the
  // remainder still walks resolvePath's escape checks, so a ".." can not
  // climb past the preopen root.
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

// gzopen snapshots the path and mode into host memory below; bound the scans so
// a guest cannot place the terminator at the far end of a multi-GB linear
// memory and force a matching host allocation. Real paths stay under PATH_MAX
// and zlib modes are only a few characters.
constexpr uint64_t MaxGZOpenPathLen = 4096;
constexpr uint64_t MaxGZOpenModeLen = 63;

Expect<uint32_t> WasmEdgeZlibGZOpen::body(const Runtime::CallingFrame &Frame,
                                          uint32_t PathPtr, uint32_t ModePtr) {
  MEMINST_CHECK(MemInst, Frame, 0)

  // gz_open answers a null path with a null handle before it parses the mode
  // or touches the filesystem, and wasm address 0 is the guest's NULL, not
  // the start of a path string.
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
  // Copy the guest path and mode into host memory before parsing or opening:
  // the mode is checked for rights and then re-parsed by zlib's gz_open, and
  // with shared linear memory another thread could rewrite the bytes in
  // between (e.g. "r" -> "w") to open with rights that were never granted.
  // Build the copies from the validated length so no fresh scan reaches past
  // the range checked above.
  const std::string Path(PathStr->data(), PathStr->size());
  const std::string Mode(ModeStr->data(), ModeStr->size());

  // Resolve the path through WASI's preopen capability layer when a WASI module
  // is configured. Distinguish the two ways getWasiEnviron declines: with no
  // WASI module at all there is no sandbox to honor, so fall back to a direct
  // host open (matching the FStream convention) with a full-capability handle;
  // but a WASI module that is present yet unrecognized (not Host::WasiModule --
  // a custom or stub wasi_snapshot_preview1) means the guest expects a sandbox
  // whose capabilities we cannot read, so fail closed rather than open a raw
  // host path outside it.
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
    // A null gzFile is zlib's ordinary open failure (e.g. a missing file or a
    // path outside the sandbox); surface it to the guest as a null handle
    // instead of trapping. No live handle is ever 0 because NextGZFile starts
    // at sizeof(gzFile).
    return UINT32_C(0);
  }

  const uint32_t NewWasmGZFile = Env.NextGZFile++;
  Env.GZFileMap.emplace(NewWasmGZFile, Entry);

  return NewWasmGZFile;
}

Expect<uint32_t> WasmEdgeZlibGZDOpen::body(const Runtime::CallingFrame &,
                                           int32_t, uint32_t) {
  // TODO(zlib): gzdopen is disabled pending an fd-lifecycle rework: bridging
  // transfers fd ownership to the gzFile, and that owned WASI fd cannot yet be
  // released safely at environment teardown without an explicit gzclose.
  // Refuse with a null handle -- zlib's ordinary gzdopen failure -- without
  // touching the descriptor, so the guest keeps its fd. Re-enable by restoring
  // the bridge here (see the wasiFdToGZ path used by gzopen).
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

  // zlib refuses a size >= 2^31 (it must be able to double it) before
  // recording anything, so keep that -1 answer instead of clamping the
  // request into an accepted one; answering here also keeps the behavior
  // uniform when the host links zlib-ng, which would silently clamp and
  // accept. Below that, clamp a guest-declared size to a sane maximum:
  // gzbuffer makes zlib allocate three times this size on the first I/O, so
  // an unclamped request just under 2 GiB would exhaust host memory.
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

  // gzread refuses a length that does not fit in a signed int -- it returns -1
  // and sets Z_STREAM_ERROR without touching the buffer -- so let zlib answer
  // instead of trapping on a range that could never be read anyway (0x7fffffff
  // is INT_MAX).
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
  // A wasm32 guest's z_size_t is 32-bit, so a size*nitems product that does
  // not fit maps to zlib's documented overflow: nothing is read, zero comes
  // back, and gz_error records a sticky Z_STREAM_ERROR that blocks further
  // I/O until gzclearerr. Reproduce that exact path on the host stream -- its
  // 64-bit gzfread would otherwise attempt the multi-GB read -- with a
  // product that also overflows the host z_size_t: the buffer is never
  // touched, and a wrong-mode or already-errored handle still answers first,
  // as it would natively.
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

  // gzwrite refuses a length that does not fit in a signed int -- it returns
  // 0 and sets Z_DATA_ERROR without touching the buffer -- so as with gzread
  // let zlib answer instead of trapping on a range that could never be
  // written anyway (0x7fffffff is INT_MAX).
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
  // As with gzfread, a wasm32 size*nitems overflow is zlib's documented
  // zero-return with a sticky Z_STREAM_ERROR; reproduce it on the host stream
  // with a product that also overflows the host z_size_t, which never touches
  // the buffer.
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

  // Write the validated bytes directly rather than copying them into a host
  // string: gzputs would re-scan guest memory with strlen (a TOCTOU under
  // shared memory), and snapshotting a guest-chosen length could force a
  // multi-GB host allocation. gzwrite takes the measured length, so zlib never
  // rescans and nothing is materialized. gzputs refuses a non-writing or
  // errored handle before accepting even a zero-length string, so the empty
  // case asks the real gzputs with a host-owned empty string, and a short
  // write reports -1 (not the byte count) exactly as gzputs does.
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

  // WASI's capability checks never see zlib's lseek on the duplicated
  // descriptor, so refuse repositioning when FD_SEEK was absent at open. Only
  // read handles need the gate: write-mode gzseek never repositions the
  // descriptor (zlib answers -1 to backward seeks and compresses zeros for
  // forward seeks), while a read-mode seek may lseek at zlib's discretion.
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

  // gzrewind repositions the descriptor with lseek only for read handles
  // (zlib answers -1 in write mode before any lseek), so enforce FD_SEEK the
  // same way as gzseek above.
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

  // gzoffset queries the descriptor position with lseek; refuse when FD_TELL
  // was absent at open. gztell stays available regardless: zlib computes it
  // from its own counters without touching the descriptor, as on a pipe.
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

  // Z_STREAM_ERROR means the handle was the wrong mode and was NOT freed; keep
  // it (and the guest fd it owns) so the environment destructor can still
  // reclaim it.
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

  // Z_STREAM_ERROR means the handle was the wrong mode and was NOT freed; keep
  // it (and the guest fd it owns) so the environment destructor can still
  // reclaim it.
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
  // See WasmEdgeZlibInflateInit2 for the windowBits-to-wrap mapping.
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
  // zlib rejects a null or major-mismatched version before it examines any
  // other argument, so mirror that order and answer Z_VERSION_ERROR here
  // rather than resolve (and possibly trap on) the window span first.
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
    // A guest null window is zlib's Z_NULL and an out-of-range windowBits is
    // equally rejected before the window is touched, so neither resolves a
    // guest pointer (wasm address 0 must not stand in for Z_NULL here).
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
  // inflateCodesUsed's bad-state answer is (unsigned long)-1.
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

  // inflateResetKeep detaches zlib's gzip-header pointer like inflateReset;
  // drop the stale snapshot (see WasmEdgeZlibInflateReset) and the
  // dictionary-wait flag with it (the stream is back at the header stage).
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

  // deflateResetKeep also returns the stream to INIT_STATE, so clear the
  // started flag that gates deflateSetDictionary.
  if (ZRes == Z_OK) {
    HostZStreamIt->second.DeflateStarted = false;
  }

  return ZRes;
}

} // namespace Host
} // namespace WasmEdge
