// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/component/canonical_abi.h"

#include "canonical_abi_internal.h"
#include "common/spdlog.h"
#include "executor/component/async_runtime.h"
#include "executor/component/executor.h"
#include "executor/executor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Executor {
namespace Component {
namespace CanonicalABI {

using namespace std::literals;

namespace {

// Resolve the runtime resource identity for an own/borrow handle type.
const Runtime::Instance::Component::ResourceTypeRT *
handleResource(const CanonCtx &Cx, uint32_t TypeIdx) noexcept {
  if (Cx.ResourceResolver) {
    return Cx.ResourceResolver(TypeIdx);
  }
  return Cx.CompInst != nullptr ? Cx.CompInst->getTypeResource(TypeIdx)
                                : nullptr;
}
} // namespace

// Transferring ownership out of the instance removes the handle.
Expect<uint64_t> liftOwnHandle(const CanonCtx &Cx, uint32_t TypeIdx,
                               uint32_t Idx) noexcept {
  const auto *RT = handleResource(Cx, TypeIdx);
  if (Cx.CompInst == nullptr || RT == nullptr) {
    // No table context (unit ABI tests): pass the raw value through.
    return Idx;
  }
  auto *Slot = Cx.CompInst->handles().handleGet(Idx);
  if (Slot == nullptr) {
    spdlog::error(ErrCode::Value::ComponentHandleUnknown);
    spdlog::error("    canonical ABI: unknown handle index {}"sv, Idx);
    return Unexpect(ErrCode::Value::ComponentHandleUnknown);
  }
  if (Slot->RT != RT) {
    spdlog::error(ErrCode::Value::ComponentHandleWrongType);
    spdlog::error(
        "    canonical ABI: handle index {} used with the wrong type"sv, Idx);
    return Unexpect(ErrCode::Value::ComponentHandleWrongType);
  }
  if (!Slot->Own || Slot->Lends != 0) {
    spdlog::error(ErrCode::Value::ComponentResourceBorrowed);
    spdlog::error("    canonical ABI: own handle {} is lent or not owned"sv,
                  Idx);
    return Unexpect(ErrCode::Value::ComponentResourceBorrowed);
  }
  return Cx.CompInst->handles().handleRemove(Idx)->Rep;
}

// The borrowed handle stays; only the representation travels for the
// duration of the call.
Expect<uint64_t> liftBorrowHandle(const CanonCtx &Cx, uint32_t TypeIdx,
                                  uint32_t Idx) noexcept {
  const auto *RT = handleResource(Cx, TypeIdx);
  if (Cx.CompInst == nullptr || RT == nullptr) {
    return Idx;
  }
  // The owning instance passes representations directly for borrows.
  if (RT->Impl == Cx.CompInst) {
    return Idx;
  }
  auto *Slot = Cx.CompInst->handles().handleGet(Idx);
  if (Slot == nullptr) {
    spdlog::error(ErrCode::Value::ComponentHandleUnknown);
    spdlog::error("    canonical ABI: unknown handle index {}"sv, Idx);
    return Unexpect(ErrCode::Value::ComponentHandleUnknown);
  }
  if (Slot->RT != RT) {
    spdlog::error(ErrCode::Value::ComponentHandleWrongType);
    spdlog::error(
        "    canonical ABI: handle index {} used with the wrong type"sv, Idx);
    return Unexpect(ErrCode::Value::ComponentHandleWrongType);
  }
  // The borrow lends the handle for the duration of the call.
  Slot->Lends += 1;
  if (Cx.LiftedBorrows != nullptr) {
    Cx.LiftedBorrows->emplace_back(Cx.CompInst, Idx);
  }
  return Slot->Rep;
}

// Entering an instance inserts a table entry for the handle.
uint32_t lowerHandle(const CanonCtx &Cx, uint32_t TypeIdx, uint64_t Rep,
                     bool Own) noexcept {
  const auto *RT = handleResource(Cx, TypeIdx);
  if (Cx.CompInst == nullptr || RT == nullptr) {
    return Rep;
  }
  // Borrows lowered into the owning instance receive the representation
  // directly.
  if (!Own && RT->Impl == Cx.CompInst) {
    return Rep;
  }
  if (!Own && Cx.BorrowTask != nullptr) {
    // The receiving task scopes the borrow.
    Cx.BorrowTask->NumBorrows += 1;
    return Cx.CompInst->handles().handleAdd(RT, Rep, Own, Cx.BorrowTask);
  }
  return Cx.CompInst->handles().handleAdd(RT, Rep, Own);
}

// Transferring the readable end of a stream or future removes it from the
// sender's table.
Expect<std::shared_ptr<void>> liftCopyEnd(const CanonCtx &Cx, bool IsStream,
                                          uint32_t Idx) noexcept {
  const auto WantKind =
      IsStream ? Runtime::Instance::Component::WaitableObj::Kind::StreamRead
               : Runtime::Instance::Component::WaitableObj::Kind::FutureRead;
  auto *W = Cx.CompInst != nullptr ? Cx.CompInst->handles().waitableGet(Idx)
                                   : nullptr;
  if (W == nullptr || W->getKind() != WantKind) {
    spdlog::error(ErrCode::Value::ComponentHandleUnknown);
    spdlog::error("    unknown handle index {}"sv, Idx);
    return Unexpect(ErrCode::Value::ComponentHandleUnknown);
  }
  auto *E = static_cast<Runtime::Instance::Component::CopyEndObj *>(W);
  if (E->St == Runtime::Instance::Component::CopyState::Done && E->DoneByDrop) {
    const auto Code = IsStream
                          ? ErrCode::Value::ComponentStreamLiftAfterDrop
                          : ErrCode::Value::ComponentFutureLiftAfterSuccess;
    spdlog::error(Code);
    spdlog::error("    cannot lift {} after being notified that the writable "
                  "end dropped"sv,
                  IsStream ? "stream" : "future");
    return Unexpect(Code);
  }
  if (E->inWaitableSet()) {
    const auto Code = IsStream ? ErrCode::Value::ComponentStreamLiftInSet
                               : ErrCode::Value::ComponentFutureLiftInSet;
    spdlog::error(Code);
    spdlog::error("    cannot lift {} while it's in a waitable set"sv,
                  IsStream ? "stream" : "future");
    return Unexpect(Code);
  }
  if (E->St == Runtime::Instance::Component::CopyState::Done) {
    const auto Code = IsStream
                          ? ErrCode::Value::ComponentStreamLiftAfterDrop
                          : ErrCode::Value::ComponentFutureLiftAfterSuccess;
    spdlog::error(Code);
    if (IsStream) {
      spdlog::error("    cannot lift stream after being notified that the "
                    "writable end dropped"sv);
    } else {
      spdlog::error("    cannot lift future after previous read succeeded"sv);
    }
    return Unexpect(Code);
  }
  if (E->copying()) {
    spdlog::error(ErrCode::Value::ComponentStreamRemoveBusy);
    spdlog::error("    cannot remove busy stream"sv);
    return Unexpect(ErrCode::Value::ComponentStreamRemoveBusy);
  }
  auto Shared = E->Shared;
  Cx.CompInst->handles().waitableRemove(Idx);
  return std::static_pointer_cast<void>(Shared);
}

// Entering an instance mints a fresh readable end over the shared object.
Expect<uint32_t> lowerCopyEnd(const CanonCtx &Cx, bool IsStream,
                              const std::shared_ptr<void> &SharedV) noexcept {
  if (Cx.CompInst == nullptr || !SharedV) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    cannot lower a stream or future without a table"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  auto Shared =
      std::static_pointer_cast<Runtime::Instance::Component::SharedCopyObj>(
          SharedV);
  auto End = std::make_shared<Runtime::Instance::Component::CopyEndObj>(
      IsStream ? Runtime::Instance::Component::WaitableObj::Kind::StreamRead
               : Runtime::Instance::Component::WaitableObj::Kind::FutureRead,
      Shared);
  auto *EndP = End.get();
  const uint32_t Idx = Cx.CompInst->handles().waitableAdd(std::move(End));
  EndP->TableIdx = Idx;
  return Idx;
}

// Invoke the guest's `realloc` core function and return the freshly
// allocated address. An invoke failure is unrecoverable here, so it traps.
Expect<uint64_t> callRealloc(const CanonCtx &Cx, uint64_t OldPtr,
                             uint64_t OldSize, uint32_t Align,
                             uint64_t NewSize) noexcept {
  if (Cx.Exec == nullptr || Cx.Realloc == nullptr) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    canonical ABI: realloc required but not provided"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  // realloc takes the address type of the memory it serves.
  auto Arg = [&Cx](uint64_t V) {
    return Cx.memory64() ? ValVariant(V) : ValVariant(static_cast<uint32_t>(V));
  };
  std::array<ValVariant, 4> Args{Arg(OldPtr), Arg(OldSize), Arg(Align),
                                 Arg(NewSize)};
  auto ParamTypes = Cx.Realloc->getFuncType().getParamTypes();
  EXPECTED_TRY(auto Res, Cx.Exec->core().invoke(Cx.Realloc, Args, ParamTypes));
  if (Res.empty()) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    canonical ABI: realloc returned no value"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  const uint64_t Ptr = Cx.memory64() ? Res[0].first.get<uint64_t>()
                                     : Res[0].first.get<uint32_t>();
  // The returned pointer is alignment-checked before it is bounds-checked,
  // and 0 is a valid allocation.
  if (Align > 1U && (Ptr & uint64_t(Align - 1U)) != 0U) {
    spdlog::error(ErrCode::Value::ComponentPtrUnaligned);
    spdlog::error("    canonical ABI: realloc return: result not aligned"sv);
    return Unexpect(ErrCode::Value::ComponentPtrUnaligned);
  }
  if (Cx.Mem != nullptr) {
    const uint64_t End = Ptr + NewSize;
    if (End > uint64_t(Cx.Mem->getPageSize()) * 65536ULL) {
      spdlog::error(ErrCode::Value::ComponentReallocOOB);
      spdlog::error(
          "    canonical ABI: realloc return: beyond end of memory"sv);
      return Unexpect(ErrCode::Value::ComponentReallocOOB);
    }
  }
  return Ptr;
}

// An error-context travels as an index into the lifting instance's handles
// table.
Expect<ComponentValVariant> liftErrorContext(const CanonCtx &Cx,
                                             uint32_t Idx) noexcept {
  const auto *Obj = Cx.CompInst != nullptr
                        ? Cx.CompInst->handles().errorContextGet(Idx)
                        : nullptr;
  if (Obj == nullptr) {
    spdlog::error(ErrCode::Value::ComponentHandleWrongType);
    spdlog::error("    handle {} is not an error-context"sv, Idx);
    return Unexpect(ErrCode::Value::ComponentHandleWrongType);
  }
  return makeComponentVal(ErrorContextVal{*Obj});
}

Expect<uint32_t> lowerErrorContext(const CanonCtx &Cx,
                                   const ComponentValVariant &V) noexcept {
  const auto *VC = std::get_if<std::shared_ptr<ValComp>>(&V);
  const auto *EC = (VC != nullptr && *VC)
                       ? std::get_if<ErrorContextVal>(&(*VC)->V)
                       : nullptr;
  if (EC == nullptr || Cx.CompInst == nullptr) {
    spdlog::error(ErrCode::Value::ComponentHandleWrongType);
    spdlog::error("    expected an error-context value"sv);
    return Unexpect(ErrCode::Value::ComponentHandleWrongType);
  }
  return Cx.CompInst->handles().errorContextAdd(EC->Message);
}

uint32_t resolveVariantCase(const VariantVal &V,
                            const AST::Component::VariantTy &T) noexcept {
  if (!V.Label.empty()) {
    for (size_t I = 0; I < T.Cases.size(); ++I) {
      if (T.Cases[I].first == V.Label) {
        return static_cast<uint32_t>(I);
      }
    }
  }
  return V.Case;
}

uint32_t resolveEnumCase(const EnumVal &E,
                         const AST::Component::EnumTy &T) noexcept {
  if (!E.Label.empty()) {
    for (size_t I = 0; I < T.Labels.size(); ++I) {
      if (T.Labels[I] == E.Label) {
        return static_cast<uint32_t>(I);
      }
    }
  }
  return E.Case;
}

uint64_t packFlags(const FlagsVal &F,
                   const AST::Component::FlagsTy &T) noexcept {
  uint64_t Packed = 0;
  if (F.Bits.empty() && !F.SetLabels.empty()) {
    for (const auto &Label : F.SetLabels) {
      for (size_t I = 0; I < T.Labels.size(); ++I) {
        if (T.Labels[I] == Label) {
          Packed |= (1ull << I);
        }
      }
    }
    return Packed;
  }
  for (size_t I = 0; I < F.Bits.size(); ++I) {
    if (F.Bits[I]) {
      Packed |= (1ull << I);
    }
  }
  return Packed;
}

// Every NaN crossing the canonical ABI collapses to the canonical quiet-NaN
// bit pattern, so float values stay reproducible across the boundary.
constexpr uint32_t kCanonicalF32NaNBits = 0x7fc00000u;
constexpr uint64_t kCanonicalF64NaNBits = 0x7ff8000000000000ull;

float canonicalizeNaN32(float F) noexcept {
  if (std::isnan(F)) {
    float C = 0.f;
    std::memcpy(&C, &kCanonicalF32NaNBits, sizeof(C));
    return C;
  }
  return F;
}

double canonicalizeNaN64(double F) noexcept {
  if (std::isnan(F)) {
    double C = 0.;
    std::memcpy(&C, &kCanonicalF64NaNBits, sizeof(C));
    return C;
  }
  return F;
}

namespace {

// Wraps MemoryInstance::loadValue<T,N> with a runtime byte width, keeping
// the template-arg comma out of EXPECTED_TRY's macro expansion.
template <typename T>
Expect<void> loadN(Runtime::Instance::MemoryInstance &Mem, uint32_t Bytes,
                   T &Val, uint64_t Off) noexcept {
  switch (Bytes) {
  case 1:
    return Mem.template loadValue<T, 1>(Val, Off);
  case 2:
    return Mem.template loadValue<T, 2>(Val, Off);
  case 3:
    return Mem.template loadValue<T, 3>(Val, Off);
  case 4:
    return Mem.template loadValue<T, 4>(Val, Off);
  default:
    assumingUnreachable();
  }
}

template <typename T>
Expect<void> storeN(Runtime::Instance::MemoryInstance &Mem, uint32_t Bytes,
                    T Val, uint64_t Off) noexcept {
  switch (Bytes) {
  case 1:
    return Mem.template storeValue<T, 1>(Val, Off);
  case 2:
    return Mem.template storeValue<T, 2>(Val, Off);
  case 3:
    return Mem.template storeValue<T, 3>(Val, Off);
  case 4:
    return Mem.template storeValue<T, 4>(Val, Off);
  default:
    assumingUnreachable();
  }
}
} // namespace

// Trap with a diagnostic naming the offending region.
Expect<uint64_t> loadPtr(const CanonCtx &Cx, uint64_t Ptr) noexcept {
  assuming(Cx.Mem != nullptr);
  if (Cx.memory64()) {
    uint64_t V = 0;
    EXPECTED_TRY(Cx.Mem->loadValue<uint64_t>(V, Ptr));
    return V;
  }
  uint32_t V = 0;
  EXPECTED_TRY(Cx.Mem->loadValue<uint32_t>(V, Ptr));
  return static_cast<uint64_t>(V);
}

Expect<void> storePtr(const CanonCtx &Cx, uint64_t V, uint64_t Ptr) noexcept {
  assuming(Cx.Mem != nullptr);
  if (Cx.memory64()) {
    return Cx.Mem->storeValue<uint64_t>(V, Ptr);
  }
  return Cx.Mem->storeValue<uint32_t>(static_cast<uint32_t>(V), Ptr);
}

[[nodiscard]] Expect<void> trapMemoryOOB(const std::string_view What,
                                         uint64_t Ptr, uint64_t Len) noexcept {
  spdlog::error(ErrCode::Value::MemoryOutOfBounds);
  spdlog::error("    canonical ABI: {} at ptr=0x{:x} len={} out of bounds"sv,
                What, Ptr, Len);
  return Unexpect(ErrCode::Value::MemoryOutOfBounds);
}

[[nodiscard]] Expect<void> trapDataInvalid(const std::string_view Msg,
                                           ErrCode::Value Code) noexcept {
  spdlog::error(Code);
  spdlog::error("    canonical ABI: {}"sv, Msg);
  return Unexpect(Code);
}

// Trap on >0x10FFFF or a UTF-16 surrogate, wherever a char comes from the
// guest.
[[nodiscard]] Expect<void> validateUSV(uint32_t I) noexcept {
  if (I >= 0x110000u) {
    return trapDataInvalid(
        "invalid `char` bit pattern: code point out of range",
        ErrCode::Value::ComponentCharInvalid);
  }
  if (I >= 0xD800u && I <= 0xDFFFu) {
    return trapDataInvalid("invalid `char` bit pattern: surrogate",
                           ErrCode::Value::ComponentCharInvalid);
  }
  return {};
}

// Used where the char comes from a host-constructed value, so a malformed
// one is a host-side bug.
void assumeValidUSV(uint32_t I) noexcept {
  assuming(I < 0x110000u && (I < 0xD800u || I > 0xDFFFu));
}

namespace {

// ---- String transcoding ----------------------------------------------------
// The host holds component strings as UTF-8: decodeString converts the guest
// bytes in Cx.Enc into that form, and encodeString does the reverse.

// The utf16 marker of a tagged length: the high bit.
constexpr uint32_t kUtf16Tag = 0x80000000u;

// Append a Unicode scalar value as UTF-8. Caller guarantees a valid USV.
void appendUtf8(std::string &Out, uint32_t CP) noexcept {
  if (CP < 0x80u) {
    Out.push_back(static_cast<char>(CP));
  } else if (CP < 0x800u) {
    Out.push_back(static_cast<char>(0xC0u | (CP >> 6)));
    Out.push_back(static_cast<char>(0x80u | (CP & 0x3Fu)));
  } else if (CP < 0x10000u) {
    Out.push_back(static_cast<char>(0xE0u | (CP >> 12)));
    Out.push_back(static_cast<char>(0x80u | ((CP >> 6) & 0x3Fu)));
    Out.push_back(static_cast<char>(0x80u | (CP & 0x3Fu)));
  } else {
    Out.push_back(static_cast<char>(0xF0u | (CP >> 18)));
    Out.push_back(static_cast<char>(0x80u | ((CP >> 12) & 0x3Fu)));
    Out.push_back(static_cast<char>(0x80u | ((CP >> 6) & 0x3Fu)));
    Out.push_back(static_cast<char>(0x80u | (CP & 0x3Fu)));
  }
}

// Decode one UTF-8 scalar starting at S[I], advancing I past it. Traps on
// any malformed sequence.
Expect<uint32_t> decodeUtf8Scalar(std::string_view S, size_t &I) noexcept {
  const size_t N = S.size();
  const uint8_t B0 = static_cast<uint8_t>(S[I]);
  uint32_t CP = 0;
  size_t Len = 0;
  uint32_t Min = 0;
  if (B0 < 0x80u) {
    CP = B0;
    Len = 1;
    Min = 0;
  } else if ((B0 & 0xE0u) == 0xC0u) {
    CP = B0 & 0x1Fu;
    Len = 2;
    Min = 0x80u;
  } else if ((B0 & 0xF0u) == 0xE0u) {
    CP = B0 & 0x0Fu;
    Len = 3;
    Min = 0x800u;
  } else if ((B0 & 0xF8u) == 0xF0u) {
    CP = B0 & 0x07u;
    Len = 4;
    Min = 0x10000u;
  } else {
    EXPECTED_TRY(trapDataInvalid("invalid UTF-8 lead byte",
                                 ErrCode::Value::ComponentUTF8Invalid));
  }
  if (I + Len > N) {
    EXPECTED_TRY(trapDataInvalid("truncated UTF-8 sequence",
                                 ErrCode::Value::ComponentUTF8Incomplete));
  }
  for (size_t K = 1; K < Len; ++K) {
    const uint8_t B = static_cast<uint8_t>(S[I + K]);
    if ((B & 0xC0u) != 0x80u) {
      EXPECTED_TRY(trapDataInvalid("invalid UTF-8 continuation byte",
                                   ErrCode::Value::ComponentUTF8Invalid));
    }
    CP = (CP << 6) | (B & 0x3Fu);
  }
  if (CP < Min || CP > 0x10FFFFu || (CP >= 0xD800u && CP <= 0xDFFFu)) {
    EXPECTED_TRY(trapDataInvalid("invalid UTF-8 scalar value",
                                 ErrCode::Value::ComponentUTF8Invalid));
  }
  I += Len;
  return CP;
}

// Validate that S is well-formed UTF-8, trapping otherwise. Allocates
// nothing, so the lift/load path can hand the raw view to the host.
Expect<void> validateUtf8(std::string_view S) noexcept {
  for (size_t I = 0; I < S.size();) {
    EXPECTED_TRY(auto CP, decodeUtf8Scalar(S, I));
    static_cast<void>(CP);
  }
  return {};
}

// Decode a UTF-8 host string into Unicode scalar values, validating as we go.
// Used by the encode side where the source is the host's UTF-8 std::string.
Expect<std::vector<uint32_t>> utf8ToCodePoints(std::string_view S) noexcept {
  std::vector<uint32_t> CPs;
  for (size_t I = 0; I < S.size();) {
    EXPECTED_TRY(auto CP, decodeUtf8Scalar(S, I));
    CPs.push_back(CP);
  }
  return CPs;
}

// Decode UTF-16-LE bytes into a UTF-8 string, pairing surrogates.
Expect<std::string> utf16leToUtf8(Span<const Byte> Bytes) noexcept {
  std::string Out;
  const size_t N = Bytes.size();
  // A UTF-16 code unit expands to at most 3 UTF-8 bytes, so this never
  // under-reserves.
  Out.reserve(N / 2 * 3);
  for (size_t I = 0; I + 1 < N; I += 2) {
    uint32_t U =
        static_cast<uint32_t>(static_cast<uint8_t>(Bytes[I])) |
        (static_cast<uint32_t>(static_cast<uint8_t>(Bytes[I + 1])) << 8);
    if (U >= 0xD800u && U <= 0xDBFFu) {
      if (I + 3 >= N) {
        EXPECTED_TRY(trapDataInvalid("unpaired UTF-16 high surrogate"));
      }
      const uint32_t L =
          static_cast<uint32_t>(static_cast<uint8_t>(Bytes[I + 2])) |
          (static_cast<uint32_t>(static_cast<uint8_t>(Bytes[I + 3])) << 8);
      if (L < 0xDC00u || L > 0xDFFFu) {
        EXPECTED_TRY(trapDataInvalid("invalid UTF-16 low surrogate"));
      }
      U = 0x10000u + ((U - 0xD800u) << 10) + (L - 0xDC00u);
      I += 2;
    } else if (U >= 0xDC00u && U <= 0xDFFFu) {
      EXPECTED_TRY(trapDataInvalid("unpaired UTF-16 low surrogate"));
    }
    appendUtf8(Out, U);
  }
  return Out;
}

// Encode Unicode scalar values to UTF-16-LE bytes (surrogate pair for >U+FFFF).
std::vector<Byte>
codePointsToUtf16le(const std::vector<uint32_t> &CPs) noexcept {
  std::vector<Byte> Out;
  Out.reserve(CPs.size() * 2);
  auto push16 = [&](uint32_t U) {
    Out.push_back(static_cast<Byte>(U & 0xFFu));
    Out.push_back(static_cast<Byte>((U >> 8) & 0xFFu));
  };
  for (uint32_t CP : CPs) {
    if (CP < 0x10000u) {
      push16(CP);
    } else {
      const uint32_t V = CP - 0x10000u;
      push16(0xD800u + (V >> 10));
      push16(0xDC00u + (V & 0x3FFu));
    }
  }
  return Out;
}
} // namespace

// Decode tagged_code_units of guest bytes at Begin into a host UTF-8
// string.
Expect<std::string> decodeString(const CanonCtx &Cx, uint64_t Begin,
                                 uint64_t TaggedCodeUnits) noexcept {
  enum class WireEnc { Utf8, Utf16, Latin1 } W = WireEnc::Utf8;
  uint32_t Alignment = 1;
  uint64_t ByteLen64 = 0;
  switch (Cx.Enc) {
  case StringEncoding::UTF8:
    W = WireEnc::Utf8;
    Alignment = 1;
    ByteLen64 = TaggedCodeUnits;
    break;
  case StringEncoding::UTF16:
    W = WireEnc::Utf16;
    Alignment = 2;
    ByteLen64 = 2ull * TaggedCodeUnits;
    break;
  case StringEncoding::Latin1UTF16:
    Alignment = 2;
    if ((TaggedCodeUnits & kUtf16Tag) != 0u) {
      W = WireEnc::Utf16;
      ByteLen64 = 2ull * (TaggedCodeUnits ^ kUtf16Tag);
    } else {
      W = WireEnc::Latin1;
      ByteLen64 = TaggedCodeUnits;
    }
    break;
  }
  if (ByteLen64 > static_cast<uint64_t>(kMaxCanonByteLength)) {
    EXPECTED_TRY(trapDataInvalid("string byte length exceeds MAX"));
  }
  const uint32_t ByteLen = static_cast<uint32_t>(ByteLen64);
  if (Begin != alignTo(Begin, Alignment)) {
    EXPECTED_TRY(trapDataInvalid("unaligned pointer for string",
                                 ErrCode::Value::ComponentPtrUnaligned));
  }
  if (!Cx.Mem->checkAccessBound(Begin, ByteLen)) {
    // A cross-component adapter and the host boundary report the bounds
    // failure with different diagnostics.
    const auto Code = Cx.CrossComponent ? ErrCode::Value::ComponentStrOOB
                                        : ErrCode::Value::ComponentStrPtrLenOOB;
    spdlog::error(Code);
    spdlog::error("    canonical ABI: string at 0x{:x} len={} out of bounds"sv,
                  Begin, ByteLen);
    return Unexpect(Code);
  }
  auto SV = Cx.Mem->getStringView(Begin, ByteLen);
  switch (W) {
  case WireEnc::Utf8: {
    // Malformed input traps, so validate before handing the bytes to the
    // host, which expects well-formed UTF-8.
    EXPECTED_TRY(validateUtf8(SV));
    return std::string(SV);
  }
  case WireEnc::Latin1: {
    std::string Out;
    Out.reserve(ByteLen);
    for (size_t I = 0; I < SV.size(); ++I) {
      appendUtf8(Out, static_cast<uint8_t>(SV[I]));
    }
    return Out;
  }
  case WireEnc::Utf16:
    return utf16leToUtf8(
        Span<const Byte>{reinterpret_cast<const Byte *>(SV.data()), SV.size()});
  }
  assumingUnreachable();
}

// Encode the host UTF-8 string into a fresh guest buffer and return the
// (begin, tagged_code_units) pair for the (ptr, len) header.
Expect<std::pair<uint64_t, uint64_t>>
encodeString(const CanonCtx &Cx, const std::string &S) noexcept {
  // Realloc a buffer, bounds-check it, then copy `Bytes` in.
  auto writeBuf = [&](Span<const Byte> Bytes,
                      uint32_t Align) -> Expect<uint32_t> {
    const uint32_t Len = static_cast<uint32_t>(Bytes.size());
    // Realloc runs even for an empty payload, so a misbehaving one traps
    // deterministically.
    EXPECTED_TRY(auto Begin, callRealloc(Cx, 0u, 0u, Align, Len));
    if (Begin != alignTo(Begin, Align)) {
      EXPECTED_TRY(trapDataInvalid("unaligned pointer for string buffer",
                                   ErrCode::Value::ComponentPtrUnaligned));
    }
    if (!Cx.Mem->checkAccessBound(Begin, Len)) {
      EXPECTED_TRY(trapMemoryOOB("string payload (post-realloc)", Begin, Len));
    }
    EXPECTED_TRY(Cx.Mem->setBytes(Bytes, Begin, 0u, Len));
    return Begin;
  };

  // Encode the code points as UTF-16-LE into a fresh 2-aligned buffer; `Tag`
  // sets the high-bit utf16 marker of the latin1+utf16 fallback.
  auto writeUtf16 = [&](const std::vector<uint32_t> &CPs,
                        bool Tag) -> Expect<std::pair<uint32_t, uint32_t>> {
    auto Bytes = codePointsToUtf16le(CPs);
    const uint32_t Units = static_cast<uint32_t>(Bytes.size() / 2);
    EXPECTED_TRY(auto Begin, writeBuf(Bytes, 2u));
    return std::make_pair(Begin, Tag ? (Units | kUtf16Tag) : Units);
  };

  switch (Cx.Enc) {
  case StringEncoding::UTF8: {
    // utf8 -> utf8 is a plain byte copy.
    Span<const Byte> Bytes{reinterpret_cast<const Byte *>(S.data()), S.size()};
    EXPECTED_TRY(auto Begin, writeBuf(Bytes, 1u));
    return std::make_pair(Begin, static_cast<uint32_t>(S.size()));
  }
  case StringEncoding::UTF16: {
    // utf8 -> utf16: code units = bytes / 2.
    EXPECTED_TRY(auto CPs, utf8ToCodePoints(S));
    return writeUtf16(CPs, /*Tag=*/false);
  }
  case StringEncoding::Latin1UTF16: {
    // latin1 when every scalar fits in a byte, else utf16 with the high-bit
    // tag; the buffer is 2-aligned either way.
    EXPECTED_TRY(auto CPs, utf8ToCodePoints(S));
    std::vector<Byte> Latin1;
    Latin1.reserve(CPs.size());
    bool AllLatin1 = true;
    for (uint32_t CP : CPs) {
      if (CP >= 0x100u) {
        AllLatin1 = false;
        break;
      }
      Latin1.push_back(static_cast<Byte>(CP));
    }
    if (AllLatin1) {
      EXPECTED_TRY(auto Begin, writeBuf(Latin1, 2u));
      return std::make_pair(Begin, static_cast<uint32_t>(CPs.size()));
    }
    return writeUtf16(CPs, /*Tag=*/true);
  }
  }
  assumingUnreachable();
}

namespace {

// Load a primitive at Ptr.
Expect<ComponentValVariant> loadPrim(const CanonCtx &Cx, uint64_t Ptr,
                                     AST::Component::PrimValType PVT) noexcept {
  assuming(Cx.Mem != nullptr);
  using P = AST::Component::PrimValType;
  switch (PVT) {
  case P::Bool: {
    // 0 → false, else true.
    uint32_t I = 0;
    EXPECTED_TRY(loadN<uint32_t>(*Cx.Mem, 1, I, Ptr));
    return ComponentValVariant{I != 0};
  }
  case P::S8: {
    int32_t V = 0;
    EXPECTED_TRY(loadN<int32_t>(*Cx.Mem, 1, V, Ptr));
    return ComponentValVariant{static_cast<int8_t>(V)};
  }
  case P::U8: {
    uint32_t V = 0;
    EXPECTED_TRY(loadN<uint32_t>(*Cx.Mem, 1, V, Ptr));
    return ComponentValVariant{static_cast<uint8_t>(V)};
  }
  case P::S16: {
    int32_t V = 0;
    EXPECTED_TRY(loadN<int32_t>(*Cx.Mem, 2, V, Ptr));
    return ComponentValVariant{static_cast<int16_t>(V)};
  }
  case P::U16: {
    uint32_t V = 0;
    EXPECTED_TRY(loadN<uint32_t>(*Cx.Mem, 2, V, Ptr));
    return ComponentValVariant{static_cast<uint16_t>(V)};
  }
  case P::S32: {
    int32_t V = 0;
    EXPECTED_TRY(Cx.Mem->loadValue<int32_t>(V, Ptr));
    return ComponentValVariant{V};
  }
  case P::U32: {
    uint32_t V = 0;
    EXPECTED_TRY(Cx.Mem->loadValue<uint32_t>(V, Ptr));
    return ComponentValVariant{V};
  }
  case P::S64: {
    int64_t V = 0;
    EXPECTED_TRY(Cx.Mem->loadValue<int64_t>(V, Ptr));
    return ComponentValVariant{V};
  }
  case P::U64: {
    uint64_t V = 0;
    EXPECTED_TRY(Cx.Mem->loadValue<uint64_t>(V, Ptr));
    return ComponentValVariant{V};
  }
  case P::F32: {
    // Canonicalize NaN on load.
    float V = 0.f;
    EXPECTED_TRY(Cx.Mem->loadValue<float>(V, Ptr));
    return ComponentValVariant{canonicalizeNaN32(V)};
  }
  case P::F64: {
    // Canonicalize NaN on load.
    double V = 0.;
    EXPECTED_TRY(Cx.Mem->loadValue<double>(V, Ptr));
    return ComponentValVariant{canonicalizeNaN64(V)};
  }
  case P::Char: {
    uint32_t V = 0;
    EXPECTED_TRY(Cx.Mem->loadValue<uint32_t>(V, Ptr));
    EXPECTED_TRY(validateUSV(V));
    return ComponentValVariant{V};
  }
  case P::String: {
    // Read (begin, tagged_code_units), then decode per the canon
    // string-encoding option.
    EXPECTED_TRY(auto Begin, loadPtr(Cx, Ptr));
    EXPECTED_TRY(auto Tagged, loadPtr(Cx, Ptr + Cx.ptrSize()));
    EXPECTED_TRY(auto Str, decodeString(Cx, Begin, Tagged));
    return ComponentValVariant{std::move(Str)};
  }
  case P::ErrorContext: {
    uint32_t Idx = 0;
    EXPECTED_TRY(Cx.Mem->loadValue<uint32_t>(Idx, Ptr));
    return liftErrorContext(Cx, Idx);
  }
  default:
    spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
    spdlog::error("    canonical ABI: load of unknown prim 0x{:02x}"sv,
                  static_cast<uint8_t>(PVT));
    return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
  }
}
} // namespace

Expect<ComponentValVariant> load(const CanonCtx &Cx, uint64_t Ptr,
                                 const ComponentValType &T) noexcept {
  using TC = ComponentTypeCode;
  const TC Code = T.getCode();

  if (Code == TC::TypeIndex) {
    const auto *DT = resolveDefType(Cx, T.getTypeIndex());
    if (DT == nullptr || !DT->isDefValType()) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error(
          "    canonical ABI: type index {} does not refer to a value type"sv,
          T.getTypeIndex());
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    return loadDef(Cx, Ptr, DT->getDefValType());
  }

  return loadPrim(
      Cx, Ptr,
      static_cast<AST::Component::PrimValType>(static_cast<uint8_t>(Code)));
}

Expect<ComponentValVariant>
loadDef(const CanonCtx &Cx, uint64_t Ptr,
        const AST::Component::DefValType &T) noexcept {
  if (T.isPrimValType()) {
    return loadPrim(Cx, Ptr, T.getPrimValType());
  }

  if (T.isRecordTy()) {
    RecordVal R;
    uint32_t Off = Ptr;
    for (const auto &F : T.getRecord().LabelTypes) {
      EXPECTED_TRY(auto A, alignment(Cx, F.getValType()));
      Off = alignTo(Off, A);
      EXPECTED_TRY(auto V, load(Cx, Off, F.getValType()));
      R.Fields.emplace_back(std::string(F.getLabel()), std::move(V));
      EXPECTED_TRY(auto S, elemSize(Cx, F.getValType()));
      Off += S;
    }
    return makeComponentVal(std::move(R));
  }

  if (T.isTupleTy()) {
    TupleVal Tu;
    uint32_t Off = Ptr;
    for (const auto &V : T.getTuple().Types) {
      EXPECTED_TRY(auto A, alignment(Cx, V));
      Off = alignTo(Off, A);
      EXPECTED_TRY(auto Val, load(Cx, Off, V));
      Tu.Values.push_back(std::move(Val));
      EXPECTED_TRY(auto S, elemSize(Cx, V));
      Off += S;
    }
    return makeComponentVal(std::move(Tu));
  }

  if (T.isVariantTy()) {
    const auto &Vt = T.getVariant();
    const uint32_t NumCases = static_cast<uint32_t>(Vt.Cases.size());
    const uint32_t DiscSize = discriminantSize(NumCases);
    uint32_t Case = 0;
    EXPECTED_TRY(loadN<uint32_t>(*Cx.Mem, DiscSize, Case, Ptr));
    if (Case >= NumCases) {
      EXPECTED_TRY(
          trapDataInvalid("invalid variant discriminant",
                          ErrCode::Value::ComponentDiscriminantInvalid));
    }
    VariantVal VV;
    VV.Case = Case;
    if (Vt.Cases[Case].second.has_value()) {
      EXPECTED_TRY(auto MaxAlign, maxCaseAlignment(Cx, Vt.Cases));
      const uint32_t PayloadOff = alignTo(Ptr + DiscSize, MaxAlign);
      EXPECTED_TRY(auto PV, load(Cx, PayloadOff, *Vt.Cases[Case].second));
      VV.Payload = std::move(PV);
    }
    return makeComponentVal(std::move(VV));
  }

  if (T.isOptionTy()) {
    // option<T> = variant{none(0) | some(T)(1)}: disc 1B.
    uint32_t Disc = 0;
    EXPECTED_TRY(loadN<uint32_t>(*Cx.Mem, 1, Disc, Ptr));
    if (Disc >= 2u) {
      EXPECTED_TRY(
          trapDataInvalid("invalid variant discriminant for option",
                          ErrCode::Value::ComponentDiscriminantInvalid));
    }
    OptionVal OV;
    if (Disc == 1u) {
      EXPECTED_TRY(auto A, alignment(Cx, T.getOption().ValTy));
      const uint32_t PayloadOff = alignTo(Ptr + 1u, A);
      EXPECTED_TRY(auto PV, load(Cx, PayloadOff, T.getOption().ValTy));
      OV.Value = std::move(PV);
    }
    return makeComponentVal(std::move(OV));
  }

  if (T.isResultTy()) {
    // result<T,E> = variant{ok(0)(T?) | err(1)(E?)}: disc 1B.
    const auto &R = T.getResult();
    uint32_t Disc = 0;
    EXPECTED_TRY(loadN<uint32_t>(*Cx.Mem, 1, Disc, Ptr));
    if (Disc >= 2u) {
      EXPECTED_TRY(
          trapDataInvalid("invalid variant discriminant for result",
                          ErrCode::Value::ComponentDiscriminantInvalid));
    }
    ResultVal RV;
    RV.IsOk = (Disc == 0u);
    const std::optional<ComponentValType> &PT = RV.IsOk ? R.ValTy : R.ErrTy;
    if (PT.has_value()) {
      uint32_t MaxAlign = 1u;
      if (R.ValTy.has_value()) {
        EXPECTED_TRY(auto A, alignment(Cx, *R.ValTy));
        MaxAlign = std::max(MaxAlign, A);
      }
      if (R.ErrTy.has_value()) {
        EXPECTED_TRY(auto A, alignment(Cx, *R.ErrTy));
        MaxAlign = std::max(MaxAlign, A);
      }
      const uint32_t PayloadOff = alignTo(Ptr + 1u, MaxAlign);
      EXPECTED_TRY(auto PV, load(Cx, PayloadOff, *PT));
      RV.Payload = std::move(PV);
    }
    return makeComponentVal(std::move(RV));
  }

  if (T.isListTy()) {
    // with-len loads len elements in place at Ptr, with no separate
    // begin/length header.
    if (T.getList().Len.has_value()) {
      return liftListFromRange(Cx, Ptr, *T.getList().Len, T.getList().ValTy);
    }
    EXPECTED_TRY(auto Begin, loadPtr(Cx, Ptr));
    EXPECTED_TRY(auto Length, loadPtr(Cx, Ptr + Cx.ptrSize()));
    EXPECTED_TRY(auto ElemAlign, alignment(Cx, T.getList().ValTy));
    EXPECTED_TRY(auto ElemSz, elemSize(Cx, T.getList().ValTy));
    // Trap when length * element size exceeds the maximum.
    uint64_t ByteLen64 =
        static_cast<uint64_t>(Length) * static_cast<uint64_t>(ElemSz);
    if (ByteLen64 > static_cast<uint64_t>(kMaxCanonByteLength)) {
      EXPECTED_TRY(trapDataInvalid("list byte length exceeds MAX"));
    }
    if (Begin != alignTo(Begin, ElemAlign)) {
      EXPECTED_TRY(trapDataInvalid("unaligned pointer for list",
                                   ErrCode::Value::ComponentPtrUnaligned));
    }
    const uint32_t ByteLen = static_cast<uint32_t>(ByteLen64);
    if (Length > 0u && !Cx.Mem->checkAccessBound(Begin, ByteLen)) {
      EXPECTED_TRY(trapMemoryOOB("list payload", Begin, ByteLen));
    }
    ListVal LV;
    LV.Elements.reserve(Length);
    for (uint32_t I = 0; I < Length; ++I) {
      EXPECTED_TRY(auto E, load(Cx, Begin + I * ElemSz, T.getList().ValTy));
      LV.Elements.push_back(std::move(E));
    }
    return makeComponentVal(std::move(LV));
  }

  if (T.isMapTy()) {
    EXPECTED_TRY(auto Begin, loadPtr(Cx, Ptr));
    EXPECTED_TRY(auto Length, loadPtr(Cx, Ptr + Cx.ptrSize()));
    return liftListFromRangeDef(Cx, Begin, Length, mapEntryType(T.getMap()));
  }

  if (T.isFlagsTy()) {
    const auto &F = T.getFlags();
    const uint32_t Labels = static_cast<uint32_t>(F.Labels.size());
    const uint32_t Bytes = (Labels + 7u) / 8u;
    uint64_t Raw = 0;
    if (Bytes > 0u) {
      // Labels are capped at 32, so at most 4 bytes.
      assuming(Bytes <= 4u);
      uint32_t V = 0;
      EXPECTED_TRY(loadN<uint32_t>(*Cx.Mem, Bytes, V, Ptr));
      Raw = V;
    }
    FlagsVal FV;
    FV.Bits.resize(Labels);
    for (uint32_t I = 0; I < Labels; ++I) {
      FV.Bits[I] = ((Raw >> I) & 1ull) != 0ull;
      if (FV.Bits[I]) {
        FV.SetLabels.push_back(F.Labels[I]);
      }
    }
    return makeComponentVal(std::move(FV));
  }

  if (T.isEnumTy()) {
    const uint32_t NumCases = static_cast<uint32_t>(T.getEnum().Labels.size());
    const uint32_t DiscSize = discriminantSize(NumCases);
    uint32_t Case = 0;
    EXPECTED_TRY(loadN<uint32_t>(*Cx.Mem, DiscSize, Case, Ptr));
    if (Case >= NumCases) {
      EXPECTED_TRY(
          trapDataInvalid("invalid variant discriminant for enum",
                          ErrCode::Value::ComponentDiscriminantInvalid));
    }
    return makeComponentVal(EnumVal{Case, {}});
  }

  // The value carries the representation; the handle leaves for own and
  // stays for borrow.
  if (T.isOwnTy()) {
    uint32_t H = 0;
    EXPECTED_TRY(Cx.Mem->loadValue<uint32_t>(H, Ptr));
    EXPECTED_TRY(uint64_t Rep, liftOwnHandle(Cx, T.getOwn().Idx, H));
    return makeComponentVal(OwnVal{Rep});
  }

  if (T.isBorrowTy()) {
    uint32_t H = 0;
    EXPECTED_TRY(Cx.Mem->loadValue<uint32_t>(H, Ptr));
    EXPECTED_TRY(uint64_t Rep, liftBorrowHandle(Cx, T.getBorrow().Idx, H));
    return makeComponentVal(BorrowVal{Rep});
  }

  if (T.isStreamTy() || T.isFutureTy()) {
    const bool IsStream = T.isStreamTy();
    uint32_t H = 0;
    EXPECTED_TRY(Cx.Mem->loadValue<uint32_t>(H, Ptr));
    EXPECTED_TRY(auto Shared, liftCopyEnd(Cx, IsStream, H));
    return makeComponentVal(StreamFutureVal{std::move(Shared), IsStream});
  }

  spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
  spdlog::error("    canonical ABI: load of gated value type"sv);
  return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
}

namespace {

Expect<void> storePrim(const CanonCtx &Cx, const ComponentValVariant &V,
                       AST::Component::PrimValType PVT, uint64_t Ptr) noexcept {
  assuming(Cx.Mem != nullptr);
  using P = AST::Component::PrimValType;
  switch (PVT) {
  case P::Bool: {
    const uint32_t B = std::get<bool>(V) ? 1u : 0u;
    return storeN<uint32_t>(*Cx.Mem, 1, B, Ptr);
  }
  case P::S8: {
    const uint32_t Bits =
        static_cast<uint32_t>(static_cast<uint8_t>(std::get<int8_t>(V)));
    return storeN<uint32_t>(*Cx.Mem, 1, Bits, Ptr);
  }
  case P::U8:
    return storeN<uint32_t>(*Cx.Mem, 1, std::get<uint8_t>(V), Ptr);
  case P::S16: {
    const uint32_t Bits =
        static_cast<uint32_t>(static_cast<uint16_t>(std::get<int16_t>(V)));
    return storeN<uint32_t>(*Cx.Mem, 2, Bits, Ptr);
  }
  case P::U16:
    return storeN<uint32_t>(*Cx.Mem, 2, std::get<uint16_t>(V), Ptr);
  case P::S32:
    return Cx.Mem->storeValue<uint32_t>(
        static_cast<uint32_t>(std::get<int32_t>(V)), Ptr);
  case P::U32:
    return Cx.Mem->storeValue<uint32_t>(std::get<uint32_t>(V), Ptr);
  case P::S64:
    return Cx.Mem->storeValue<uint64_t>(
        static_cast<uint64_t>(std::get<int64_t>(V)), Ptr);
  case P::U64:
    return Cx.Mem->storeValue<uint64_t>(std::get<uint64_t>(V), Ptr);
  case P::F32:
    // Canonicalize NaN on store.
    return Cx.Mem->storeValue<float>(canonicalizeNaN32(std::get<float>(V)),
                                     Ptr);
  case P::F64:
    // Canonicalize NaN on store.
    return Cx.Mem->storeValue<double>(canonicalizeNaN64(std::get<double>(V)),
                                      Ptr);
  case P::Char: {
    const uint32_t I = std::get<uint32_t>(V);
    assumeValidUSV(I);
    return Cx.Mem->storeValue<uint32_t>(I, Ptr);
  }
  case P::String: {
    // Encode the host string per the canon string-encoding option, then
    // store the (begin, tagged_code_units) pair at Ptr.
    EXPECTED_TRY(auto Enc, encodeString(Cx, std::get<std::string>(V)));
    EXPECTED_TRY(storePtr(Cx, Enc.first, Ptr));
    EXPECTED_TRY(storePtr(Cx, Enc.second, Ptr + Cx.ptrSize()));
    return {};
  }
  case P::ErrorContext: {
    EXPECTED_TRY(auto Idx, lowerErrorContext(Cx, V));
    return Cx.Mem->storeValue<uint32_t>(Idx, Ptr);
  }
  default:
    spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
    spdlog::error("    canonical ABI: store of unknown prim 0x{:02x}"sv,
                  static_cast<uint8_t>(PVT));
    return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
  }
}
} // namespace

Expect<void> store(const CanonCtx &Cx, const ComponentValVariant &V,
                   const ComponentValType &T, uint64_t Ptr) noexcept {
  using TC = ComponentTypeCode;
  const TC Code = T.getCode();

  if (Code == TC::TypeIndex) {
    const auto *DT = resolveDefType(Cx, T.getTypeIndex());
    if (DT == nullptr || !DT->isDefValType()) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error(
          "    canonical ABI: type index {} does not refer to a value type"sv,
          T.getTypeIndex());
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    return storeDef(Cx, V, DT->getDefValType(), Ptr);
  }

  return storePrim(
      Cx, V,
      static_cast<AST::Component::PrimValType>(static_cast<uint8_t>(Code)),
      Ptr);
}

Expect<void> storeDef(const CanonCtx &Cx, const ComponentValVariant &V,
                      const AST::Component::DefValType &T,
                      uint64_t Ptr) noexcept {
  if (T.isPrimValType()) {
    return storePrim(Cx, V, T.getPrimValType(), Ptr);
  }

  if (T.isRecordTy()) {
    const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
    assuming(VC);
    const auto &R = std::get<RecordVal>(VC->V);
    const auto &Fields = T.getRecord().LabelTypes;
    assuming(R.Fields.size() == Fields.size());
    uint32_t Off = Ptr;
    for (size_t I = 0; I < Fields.size(); ++I) {
      EXPECTED_TRY(auto A, alignment(Cx, Fields[I].getValType()));
      Off = alignTo(Off, A);
      EXPECTED_TRY(store(Cx, R.Fields[I].second, Fields[I].getValType(), Off));
      EXPECTED_TRY(auto S, elemSize(Cx, Fields[I].getValType()));
      Off += S;
    }
    return {};
  }

  if (T.isTupleTy()) {
    const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
    assuming(VC);
    const auto &Tu = std::get<TupleVal>(VC->V);
    const auto &Types = T.getTuple().Types;
    assuming(Tu.Values.size() == Types.size());
    uint32_t Off = Ptr;
    for (size_t I = 0; I < Types.size(); ++I) {
      EXPECTED_TRY(auto A, alignment(Cx, Types[I]));
      Off = alignTo(Off, A);
      EXPECTED_TRY(store(Cx, Tu.Values[I], Types[I], Off));
      EXPECTED_TRY(auto S, elemSize(Cx, Types[I]));
      Off += S;
    }
    return {};
  }

  if (T.isVariantTy()) {
    const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
    assuming(VC);
    const auto &Vv = std::get<VariantVal>(VC->V);
    const auto &Vt = T.getVariant();
    const uint32_t Case = resolveVariantCase(Vv, Vt);
    assuming(Case < Vt.Cases.size());
    const uint32_t DiscSize =
        discriminantSize(static_cast<uint32_t>(Vt.Cases.size()));
    EXPECTED_TRY(storeN<uint32_t>(*Cx.Mem, DiscSize, Case, Ptr));
    if (Vt.Cases[Case].second.has_value()) {
      assuming(Vv.Payload.has_value());
      EXPECTED_TRY(auto MaxAlign, maxCaseAlignment(Cx, Vt.Cases));
      const uint32_t PayloadOff = alignTo(Ptr + DiscSize, MaxAlign);
      EXPECTED_TRY(store(Cx, *Vv.Payload, *Vt.Cases[Case].second, PayloadOff));
    }
    return {};
  }

  if (T.isOptionTy()) {
    const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
    assuming(VC);
    const auto &O = std::get<OptionVal>(VC->V);
    const uint32_t Disc = O.Value.has_value() ? 1u : 0u;
    EXPECTED_TRY(storeN<uint32_t>(*Cx.Mem, 1, Disc, Ptr));
    if (O.Value.has_value()) {
      EXPECTED_TRY(auto A, alignment(Cx, T.getOption().ValTy));
      const uint32_t PayloadOff = alignTo(Ptr + 1u, A);
      EXPECTED_TRY(store(Cx, *O.Value, T.getOption().ValTy, PayloadOff));
    }
    return {};
  }

  if (T.isResultTy()) {
    const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
    assuming(VC);
    const auto &R = std::get<ResultVal>(VC->V);
    const auto &Rt = T.getResult();
    const uint32_t Disc = R.IsOk ? 0u : 1u;
    EXPECTED_TRY(storeN<uint32_t>(*Cx.Mem, 1, Disc, Ptr));
    const std::optional<ComponentValType> &PT = R.IsOk ? Rt.ValTy : Rt.ErrTy;
    if (PT.has_value()) {
      assuming(R.Payload.has_value());
      uint32_t MaxAlign = 1u;
      if (Rt.ValTy.has_value()) {
        EXPECTED_TRY(auto A, alignment(Cx, *Rt.ValTy));
        MaxAlign = std::max(MaxAlign, A);
      }
      if (Rt.ErrTy.has_value()) {
        EXPECTED_TRY(auto A, alignment(Cx, *Rt.ErrTy));
        MaxAlign = std::max(MaxAlign, A);
      }
      const uint32_t PayloadOff = alignTo(Ptr + 1u, MaxAlign);
      EXPECTED_TRY(store(Cx, *R.Payload, *PT, PayloadOff));
    }
    return {};
  }

  if (T.isListTy()) {
    // with-len stores each element in place at Ptr, with no realloc and no
    // (begin, length) header.
    if (T.getList().Len.has_value()) {
      const auto &L = T.getList();
      const uint32_t Len = *L.Len;
      const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
      assuming(VC);
      const auto &Lv = std::get<ListVal>(VC->V);
      assuming(Lv.Elements.size() == Len);
      const auto &ElemT = L.ValTy;
      EXPECTED_TRY(auto ElemSz, elemSize(Cx, ElemT));
      for (uint32_t I = 0; I < Len; ++I) {
        EXPECTED_TRY(store(Cx, Lv.Elements[I], ElemT, Ptr + I * ElemSz));
      }
      return {};
    }
    const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
    assuming(VC);
    const auto &Lv = std::get<ListVal>(VC->V);
    const auto &ElemT = T.getList().ValTy;
    EXPECTED_TRY(auto ElemAlign, alignment(Cx, ElemT));
    EXPECTED_TRY(auto ElemSz, elemSize(Cx, ElemT));
    const uint32_t Length = static_cast<uint32_t>(Lv.Elements.size());
    const uint64_t ByteLen64 =
        static_cast<uint64_t>(Length) * static_cast<uint64_t>(ElemSz);
    assuming(ByteLen64 <= static_cast<uint64_t>(kMaxCanonByteLength));
    const uint32_t ByteLen = static_cast<uint32_t>(ByteLen64);
    // Realloc runs even for an empty list, so a misbehaving one traps
    // deterministically.
    EXPECTED_TRY(uint32_t Begin, callRealloc(Cx, 0u, 0u, ElemAlign, ByteLen));
    if (!Cx.Mem->checkAccessBound(Begin, ByteLen)) {
      EXPECTED_TRY(
          trapMemoryOOB("list payload (post-realloc)", Begin, ByteLen));
    }
    for (uint32_t I = 0; I < Length; ++I) {
      EXPECTED_TRY(store(Cx, Lv.Elements[I], ElemT, Begin + I * ElemSz));
    }
    EXPECTED_TRY(storePtr(Cx, Begin, Ptr));
    EXPECTED_TRY(storePtr(Cx, Length, Ptr + Cx.ptrSize()));
    return {};
  }

  if (T.isMapTy()) {
    const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
    assuming(VC);
    EXPECTED_TRY(auto Range, storeListWithDefElem(Cx, std::get<ListVal>(VC->V),
                                                  mapEntryType(T.getMap())));
    EXPECTED_TRY(storePtr(Cx, Range.first, Ptr));
    EXPECTED_TRY(storePtr(Cx, Range.second, Ptr + Cx.ptrSize()));
    return {};
  }

  if (T.isFlagsTy()) {
    const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
    assuming(VC);
    const auto &F = std::get<FlagsVal>(VC->V);
    const auto &Ft = T.getFlags();
    assuming(F.Bits.empty() || F.Bits.size() == Ft.Labels.size());
    const uint32_t Bytes = static_cast<uint32_t>((Ft.Labels.size() + 7) / 8);
    const uint64_t Packed = packFlags(F, Ft);
    if (Bytes > 0u) {
      assuming(Bytes <= 4u);
      EXPECTED_TRY(
          storeN<uint32_t>(*Cx.Mem, Bytes, static_cast<uint32_t>(Packed), Ptr));
    }
    return {};
  }

  if (T.isEnumTy()) {
    const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
    assuming(VC);
    const auto &E = std::get<EnumVal>(VC->V);
    const auto &Et = T.getEnum();
    const uint32_t Case = resolveEnumCase(E, Et);
    assuming(Case < Et.Labels.size());
    const uint32_t DiscSize =
        discriminantSize(static_cast<uint32_t>(Et.Labels.size()));
    return storeN<uint32_t>(*Cx.Mem, DiscSize, Case, Ptr);
  }

  if (T.isOwnTy()) {
    const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
    assuming(VC);
    const auto &O = std::get<OwnVal>(VC->V);
    return Cx.Mem->storeValue<uint32_t>(
        lowerHandle(Cx, T.getOwn().Idx, O.Handle, true), Ptr);
  }

  if (T.isBorrowTy()) {
    const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
    assuming(VC);
    const auto &B = std::get<BorrowVal>(VC->V);
    return Cx.Mem->storeValue<uint32_t>(
        lowerHandle(Cx, T.getBorrow().Idx, B.Handle, false), Ptr);
  }

  if (T.isStreamTy() || T.isFutureTy()) {
    const auto &VC = std::get<std::shared_ptr<ValComp>>(V);
    assuming(VC);
    const auto &SF = std::get<StreamFutureVal>(VC->V);
    EXPECTED_TRY(uint32_t Idx, lowerCopyEnd(Cx, T.isStreamTy(), SF.Shared));
    return Cx.Mem->storeValue<uint32_t>(Idx, Ptr);
  }

  spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
  spdlog::error("    canonical ABI: store of gated value type"sv);
  return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
}

// Load a list payload at (Begin, Length) for element type ElemT, used by
// liftFlat once the (ptr, len) pair comes out of the flat core values.
Expect<ComponentValVariant>
liftListFromRange(const CanonCtx &Cx, uint64_t Begin, uint64_t Length,
                  const ComponentValType &ElemT) noexcept {
  EXPECTED_TRY(auto ElemAlign, alignment(Cx, ElemT));
  EXPECTED_TRY(auto ElemSz, elemSize(Cx, ElemT));
  uint64_t ByteLen64 =
      static_cast<uint64_t>(Length) * static_cast<uint64_t>(ElemSz);
  if (ByteLen64 > static_cast<uint64_t>(kMaxCanonByteLength)) {
    EXPECTED_TRY(trapDataInvalid("list byte length exceeds MAX"));
  }
  if (Begin != alignTo(Begin, ElemAlign)) {
    EXPECTED_TRY(trapDataInvalid("unaligned pointer for list",
                                 ErrCode::Value::ComponentPtrUnaligned));
  }
  const uint32_t ByteLen = static_cast<uint32_t>(ByteLen64);
  if (Length > 0u && !Cx.Mem->checkAccessBound(Begin, ByteLen)) {
    EXPECTED_TRY(trapMemoryOOB("list payload", Begin, ByteLen));
  }
  ListVal LV;
  LV.Elements.reserve(Length);
  for (uint32_t I = 0; I < Length; ++I) {
    EXPECTED_TRY(auto E, load(Cx, Begin + I * ElemSz, ElemT));
    LV.Elements.push_back(std::move(E));
  }
  return makeComponentVal(std::move(LV));
}

// The same as liftListFromRange, for an element type that has no index.
Expect<ComponentValVariant>
liftListFromRangeDef(const CanonCtx &Cx, uint64_t Begin, uint64_t Length,
                     const AST::Component::DefValType &ElemT) noexcept {
  EXPECTED_TRY(auto ElemAlign, alignmentDef(Cx, ElemT));
  EXPECTED_TRY(auto ElemSz, elemSizeDef(Cx, ElemT));
  const uint64_t ByteLen64 =
      static_cast<uint64_t>(Length) * static_cast<uint64_t>(ElemSz);
  if (ByteLen64 > static_cast<uint64_t>(kMaxCanonByteLength)) {
    EXPECTED_TRY(trapDataInvalid("list byte length exceeds MAX"));
  }
  if (Begin != alignTo(Begin, ElemAlign)) {
    EXPECTED_TRY(trapDataInvalid("unaligned pointer for list",
                                 ErrCode::Value::ComponentPtrUnaligned));
  }
  const uint32_t ByteLen = static_cast<uint32_t>(ByteLen64);
  if (Length > 0u && !Cx.Mem->checkAccessBound(Begin, ByteLen)) {
    EXPECTED_TRY(trapMemoryOOB("list payload", Begin, ByteLen));
  }
  ListVal LV;
  LV.Elements.reserve(Length);
  for (uint32_t I = 0; I < Length; ++I) {
    EXPECTED_TRY(auto E, loadDef(Cx, Begin + I * ElemSz, ElemT));
    LV.Elements.push_back(std::move(E));
  }
  return makeComponentVal(std::move(LV));
}

// Allocate and fill the payload of a list whose element type has no index,
// and return its (begin, length).
Expect<std::pair<uint64_t, uint64_t>>
storeListWithDefElem(const CanonCtx &Cx, const ListVal &Lv,
                     const AST::Component::DefValType &ElemT) noexcept {
  EXPECTED_TRY(auto ElemAlign, alignmentDef(Cx, ElemT));
  EXPECTED_TRY(auto ElemSz, elemSizeDef(Cx, ElemT));
  const uint32_t Length = static_cast<uint32_t>(Lv.Elements.size());
  const uint64_t ByteLen64 =
      static_cast<uint64_t>(Length) * static_cast<uint64_t>(ElemSz);
  assuming(ByteLen64 <= static_cast<uint64_t>(kMaxCanonByteLength));
  const uint32_t ByteLen = static_cast<uint32_t>(ByteLen64);
  EXPECTED_TRY(uint32_t Begin, callRealloc(Cx, 0u, 0u, ElemAlign, ByteLen));
  if (!Cx.Mem->checkAccessBound(Begin, ByteLen)) {
    EXPECTED_TRY(trapMemoryOOB("list payload (post-realloc)", Begin, ByteLen));
  }
  for (uint32_t I = 0; I < Length; ++I) {
    EXPECTED_TRY(storeDef(Cx, Lv.Elements[I], ElemT, Begin + I * ElemSz));
  }
  return std::make_pair(Begin, Length);
}

} // namespace CanonicalABI
} // namespace Component
} // namespace Executor
} // namespace WasmEdge
