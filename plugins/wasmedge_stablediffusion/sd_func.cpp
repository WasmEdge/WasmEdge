// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "sd_func.h"

#include "sd_abi.h"
#include "sd_backend.h"
#include "sd_guest_file.h"
#include "sd_image.h"
#include "sd_request.h"
#include "sd_session.h"

#include "common/expected.h"
#include "common/span.h"
#include "common/spdlog.h"
#include "runtime/instance/memory.h"

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {

namespace {

// Returns the ABI name of a code, for the host log.
std::string_view nameOf(ErrNo Code) noexcept {
  switch (Code) {
  case ErrNo::Success:
    return "Success"sv;
  case ErrNo::InvalidArgument:
    return "InvalidArgument"sv;
  case ErrNo::InvalidEncoding:
    return "InvalidEncoding"sv;
  case ErrNo::MissingMemory:
    return "MissingMemory"sv;
  case ErrNo::Busy:
    return "Busy"sv;
  case ErrNo::RuntimeError:
    return "RuntimeError"sv;
  case ErrNo::BufferTooSmall:
    return "BufferTooSmall"sv;
  case ErrNo::NotFound:
    return "NotFound"sv;
  case ErrNo::LoadFailed:
    return "LoadFailed"sv;
  case ErrNo::NotReadOnly:
    return "NotReadOnly"sv;
  }
  return "an unknown code"sv;
}

// Logs one line for a failed call of host function Function and returns Code.
// Why adds detail when the code alone is not clear. The line is an error line,
// except for BufferTooSmall: a guest can cause that code on purpose to get the
// size of a result.
ErrNo failure(std::string_view Function, ErrNo Code,
              std::string_view Why = {}) noexcept {
  const auto Level =
      Code == ErrNo::BufferTooSmall ? spdlog::level::debug : spdlog::level::err;
  if (Why.empty()) {
    spdlog::log(Level, "[WasmEdge-StableDiffusion] {} failed with {}"sv,
                Function, nameOf(Code));
  } else {
    spdlog::log(Level, "[WasmEdge-StableDiffusion] {} failed with {}: {}"sv,
                Function, nameOf(Code), Why);
  }
  return Code;
}

uint32_t loadU32(const uint8_t *Bytes) noexcept {
  return static_cast<uint32_t>(Bytes[0]) |
         static_cast<uint32_t>(Bytes[1]) << 8 |
         static_cast<uint32_t>(Bytes[2]) << 16 |
         static_cast<uint32_t>(Bytes[3]) << 24;
}

void storeU32(uint8_t *Bytes, uint32_t Value) noexcept {
  for (uint32_t I = 0; I < 4; ++I) {
    Bytes[I] = static_cast<uint8_t>(Value >> (8 * I));
  }
}

// Returns the bytes [Ptr, Ptr + Len) of the caller memory. Returns
// MissingMemory if the caller has no memory or the range is outside it.
Expected<Span<uint8_t>, ErrNo> guestBytes(const Runtime::CallingFrame &Frame,
                                          uint32_t Ptr, uint64_t Len) noexcept {
  const auto *Memory = Frame.getMemoryByIndex(0);
  if (Memory == nullptr || !Memory->checkAccessBound(Ptr, Len)) {
    return Unexpected<ErrNo>(ErrNo::MissingMemory);
  }
  return Memory->getSpan<uint8_t>(Ptr, Len);
}

// Returns Success when [Ptr, Ptr + Len) is inside the caller memory.
ErrNo checkInside(const Runtime::CallingFrame &Frame, uint32_t Ptr,
                  uint64_t Len) noexcept {
  return guestBytes(Frame, Ptr, Len) ? ErrNo::Success : ErrNo::MissingMemory;
}

// Returns a copy of the guest text at [Ptr, Ptr + Len). Refuses a length above
// the limit before it reads the bytes.
Expected<std::string, ErrNo> readText(const Runtime::CallingFrame &Frame,
                                      uint32_t Ptr, uint32_t Len) noexcept {
  if (Len > Limits::MaxTextBytes) {
    return Unexpected<ErrNo>(ErrNo::InvalidArgument);
  }
  const auto Bytes = guestBytes(Frame, Ptr, Len);
  if (!Bytes) {
    return Unexpected<ErrNo>(Bytes.error());
  }
  return std::string(reinterpret_cast<const char *>(Bytes->data()),
                     Bytes->size());
}

// Returns the blobs of a table at Ptr: Count pairs {ptr, len} of little-endian
// u32. Each blob must be inside the caller memory. Refuses a count above the
// limit before it reads the table.
Expected<std::vector<Span<const uint8_t>>, ErrNo>
readBlobTable(const Runtime::CallingFrame &Frame, uint32_t Ptr,
              uint32_t Count) noexcept {
  if (Count > Limits::MaxBlobs) {
    return Unexpected<ErrNo>(ErrNo::InvalidArgument);
  }
  const auto Table = guestBytes(Frame, Ptr, UINT64_C(8) * Count);
  if (!Table) {
    return Unexpected<ErrNo>(Table.error());
  }
  std::vector<Span<const uint8_t>> Blobs;
  Blobs.reserve(Count);
  for (uint32_t I = 0; I < Count; ++I) {
    const uint8_t *Entry = Table->data() + UINT64_C(8) * I;
    const auto Blob = guestBytes(Frame, loadU32(Entry), loadU32(Entry + 4));
    if (!Blob) {
      return Unexpected<ErrNo>(Blob.error());
    }
    Blobs.push_back(*Blob);
  }
  return Blobs;
}

// Decodes the images that the request refers to, directly from its blobs in
// guest memory: the input and control images to three channels, the mask to
// one. It does not decode a blob that no key refers to.
Expected<GenerationInputs, ErrNo>
decodeInputs(const GenerationRequest &Req,
             Span<const Span<const uint8_t>> Blobs) noexcept {
  struct Slot {
    const std::optional<uint32_t> &Index;
    uint32_t Channels;
    std::optional<OwnedImage> &Image;
  };
  GenerationInputs Inputs;
  for (const Slot &Input : {Slot{Req.InitImage, 3, Inputs.Init},
                            Slot{Req.MaskImage, 1, Inputs.Mask},
                            Slot{Req.ControlImage, 3, Inputs.Control}}) {
    if (!Input.Index) {
      continue;
    }
    if (*Input.Index >= Blobs.size()) {
      return Unexpected<ErrNo>(ErrNo::InvalidArgument);
    }
    auto Decoded = decodeImage(Blobs[*Input.Index], Input.Channels);
    if (!Decoded) {
      return Unexpected<ErrNo>(Decoded.error());
    }
    Input.Image.emplace(std::move(*Decoded));
  }
  return Inputs;
}

// Writes Values as consecutive little-endian u32 at Ptr. If they do not all fit
// in the caller memory, writes nothing and returns MissingMemory.
ErrNo writeU32s(const Runtime::CallingFrame &Frame, uint32_t Ptr,
                std::initializer_list<uint32_t> Values) noexcept {
  const auto Out = guestBytes(Frame, Ptr, UINT64_C(4) * Values.size());
  if (!Out) {
    return Out.error();
  }
  uint8_t *Next = Out->data();
  for (const uint32_t Value : Values) {
    storeU32(Next, Value);
    Next += 4;
  }
  return ErrNo::Success;
}

// The two output ranges of result_read in the caller memory.
struct ResultOutputs {
  Span<uint8_t> Buf;     // [BufPtr, BufPtr + BufLen)
  Span<uint8_t> Written; // the four bytes at WrittenPtr
};

// Returns the output ranges of result_read. Returns MissingMemory if one is
// outside the caller memory, and InvalidArgument if they overlap. An empty
// buffer overlaps nothing.
Expected<ResultOutputs, ErrNo> resultOutputs(const Runtime::CallingFrame &Frame,
                                             uint32_t BufPtr, uint32_t BufLen,
                                             uint32_t WrittenPtr) noexcept {
  const auto Buf = guestBytes(Frame, BufPtr, BufLen);
  const auto Written = guestBytes(Frame, WrittenPtr, 4);
  if (!Buf || !Written) {
    return Unexpected<ErrNo>(ErrNo::MissingMemory);
  }
  const uint64_t BufEnd = uint64_t{BufPtr} + BufLen;
  const uint64_t WrittenEnd = uint64_t{WrittenPtr} + 4;
  if (BufLen != 0 && WrittenPtr < BufEnd && BufPtr < WrittenEnd) {
    return Unexpected<ErrNo>(ErrNo::InvalidArgument);
  }
  return ResultOutputs{*Buf, *Written};
}

// Copies a result into the guest buffer [BufPtr, BufPtr + BufLen) and writes
// its size at WrittenPtr. Session::resultRead already found the buffer large
// enough. On an error, it writes nothing:
// - MissingMemory or InvalidArgument: see resultOutputs.
// - RuntimeError: the result is larger than the buffer.
// It runs under the session lock, so it must touch guest memory only.
ErrNo copyResult(const Runtime::CallingFrame &Frame, uint32_t BufPtr,
                 uint32_t BufLen, uint32_t WrittenPtr,
                 Span<const uint8_t> Result) noexcept {
  const auto Out = resultOutputs(Frame, BufPtr, BufLen, WrittenPtr);
  if (!Out) {
    return Out.error();
  }
  if (Result.size() > Out->Buf.size()) {
    return ErrNo::RuntimeError;
  }
  std::copy(Result.begin(), Result.end(), Out->Buf.begin());
  storeU32(Out->Written.data(), static_cast<uint32_t>(Result.size()));
  return ErrNo::Success;
}

// The backend must get no guest path. After the files are resolved, only the
// host paths name them.
void forgetGuestPaths(SessionConfig &Config) noexcept {
  for (auto *Path :
       {&Config.ModelPath, &Config.DiffusionModelPath, &Config.ClipLPath,
        &Config.ClipGPath, &Config.T5xxlPath, &Config.VaePath,
        &Config.TaesdPath, &Config.ControlNetPath, &Config.UpscalerModelPath}) {
    Path->clear();
  }
  for (auto &Embedding : Config.Embeddings) {
    Embedding.Path.clear();
  }
}

void forgetGuestPaths(GenerationRequest &Req) noexcept {
  for (auto &Lora : Req.Loras) {
    Lora.Path.clear();
  }
}

constexpr std::string_view kTooManySessions =
    "the module instance has as many sessions as it may have"sv;

// Maps a failed Backend::open to a code for the guest: InvalidArgument and
// LoadFailed stay, and every other code becomes RuntimeError.
ErrNo openFailure(ErrNo Code) noexcept {
  return Code == ErrNo::InvalidArgument || Code == ErrNo::LoadFailed
             ? Code
             : ErrNo::RuntimeError;
}

} // namespace

// These macros log one line for a failed stage and return its code from the
// host function body. Each body that uses them has a local Name, the name of
// the host function. SD_TRY binds Var to the value of an Expected.
#define SD_TRY(Var, Expr)                                                      \
  auto Var##Result = (Expr);                                                   \
  if (!Var##Result) {                                                          \
    return castErrNo(failure(Name, Var##Result.error()));                      \
  }                                                                            \
  auto &Var = *Var##Result

#define SD_TRY_CODE(Expr)                                                      \
  do {                                                                         \
    if (const auto SDTryCode = (Expr); SDTryCode != ErrNo::Success) {          \
      return castErrNo(failure(Name, SDTryCode));                              \
    }                                                                          \
  } while (false)

Expect<uint32_t> SDAbiVersion::body(const Runtime::CallingFrame &) {
  return kAbiVersion;
}

Expect<uint32_t> SDSessionOpen::body(const Runtime::CallingFrame &Frame,
                                     uint32_t ConfigPtr, uint32_t ConfigLen,
                                     uint32_t SessionPtr) {
  constexpr auto Name = "session_open"sv;
  if (!Env.hasBackend()) {
    return castErrNo(
        failure(Name, ErrNo::RuntimeError, "the module has no backend"sv));
  }
  // Take a place before any other work. The session keeps it until its
  // context is freed. A call that fails gives it back when it returns.
  auto Slot = Env.takeSlot();
  if (!Slot) {
    return castErrNo(failure(Name, ErrNo::Busy, kTooManySessions));
  }
  SD_TRY(Text, readText(Frame, ConfigPtr, ConfigLen));
  SD_TRY_CODE(checkInside(Frame, SessionPtr, 4));
  SD_TRY(Config, parseSessionConfig(Text));
  SD_TRY(Files, resolveSessionFiles(Frame, Config, Env.createdAt()));
  forgetGuestPaths(Config);
  auto Context = Env.backend().open(Config, Files);
  if (!Context) {
    return castErrNo(failure(Name, openFailure(Context.error()),
                             "the backend did not open the session"sv));
  }
  if (!*Context) {
    return castErrNo(
        failure(Name, ErrNo::RuntimeError, "the backend opened no context"sv));
  }
  const auto Id = Env.sessions().insert(
      std::make_shared<Session>(std::move(*Context), std::move(Slot)));
  if (!Id) {
    return castErrNo(
        failure(Name, ErrNo::RuntimeError, "every session id has been used"sv));
  }
  if (const auto Code = writeU32s(Frame, SessionPtr, {*Id});
      Code != ErrNo::Success) {
    Env.sessions().remove(*Id);
    return castErrNo(failure(Name, Code));
  }
  return castErrNo(ErrNo::Success);
}

Expect<uint32_t> SDSessionClose::body(const Runtime::CallingFrame &,
                                      uint32_t SessionId) {
  constexpr auto Name = "session_close"sv;
  if (!Env.sessions().remove(SessionId)) {
    return castErrNo(
        failure(Name, ErrNo::NotFound, "no open session has this id"sv));
  }
  return castErrNo(ErrNo::Success);
}

Expect<uint32_t> SDGenerate::body(const Runtime::CallingFrame &Frame,
                                  uint32_t SessionId, uint32_t ReqPtr,
                                  uint32_t ReqLen, uint32_t BlobsPtr,
                                  uint32_t BlobsLen, uint32_t CountPtr) {
  constexpr auto Name = "generate"sv;
  SD_TRY(Text, readText(Frame, ReqPtr, ReqLen));
  SD_TRY(Blobs, readBlobTable(Frame, BlobsPtr, BlobsLen));
  // A count pointer outside the memory fails here, before the generation.
  SD_TRY_CODE(checkInside(Frame, CountPtr, 4));
  SD_TRY(Req, parseGenerationRequest(Text, BlobsLen));
  const auto S = Env.sessions().get(SessionId);
  if (!S) {
    return castErrNo(
        failure(Name, ErrNo::NotFound, "no open session has this id"sv));
  }
  SD_TRY_CODE(checkAgainst(S->capabilities(), Req));
  SD_TRY(Files, resolveRequestFiles(Frame, Req, Env.createdAt()));
  forgetGuestPaths(Req);
  uint32_t Count = 0;
  SD_TRY_CODE(S->generate(
      Req, Files,
      [&Req, &Blobs]() noexcept { return decodeInputs(Req, Blobs); }, Count));
  SD_TRY_CODE(writeU32s(Frame, CountPtr, {Count}));
  return castErrNo(ErrNo::Success);
}

Expect<uint32_t> SDResultInfo::body(const Runtime::CallingFrame &Frame,
                                    uint32_t SessionId, uint32_t Index,
                                    uint32_t Format, uint32_t InfoPtr) {
  constexpr auto Name = "result_info"sv;
  const auto S = Env.sessions().get(SessionId);
  if (!S) {
    return castErrNo(
        failure(Name, ErrNo::NotFound, "no open session has this id"sv));
  }
  ResultInfo Info{};
  SD_TRY_CODE(S->resultInfo(Index, static_cast<ResultFormat>(Format), Info));
  SD_TRY_CODE(writeU32s(Frame, InfoPtr,
                        {Info.Width, Info.Height, Info.Channels, Info.Size}));
  return castErrNo(ErrNo::Success);
}

Expect<uint32_t> SDResultRead::body(const Runtime::CallingFrame &Frame,
                                    uint32_t SessionId, uint32_t Index,
                                    uint32_t Format, uint32_t BufPtr,
                                    uint32_t BufLen, uint32_t WrittenPtr) {
  constexpr auto Name = "result_read"sv;
  const auto S = Env.sessions().get(SessionId);
  if (!S) {
    return castErrNo(
        failure(Name, ErrNo::NotFound, "no open session has this id"sv));
  }
  uint32_t Size = 0;
  const auto Code = S->resultRead(
      Index, static_cast<ResultFormat>(Format), BufLen, Size,
      [&Frame, BufPtr, BufLen,
       WrittenPtr](Span<const uint8_t> Result) noexcept {
        return copyResult(Frame, BufPtr, BufLen, WrittenPtr, Result);
      });
  if (Code == ErrNo::BufferTooSmall) {
    SD_TRY(Out, resultOutputs(Frame, BufPtr, BufLen, WrittenPtr));
    storeU32(Out.Written.data(), Size);
  }
  SD_TRY_CODE(Code);
  return castErrNo(ErrNo::Success);
}

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge
