// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "loader/aot_section.h"
#include "aot/version.h"
#include "common/spdlog.h"
#include "system/allocator.h"

#include <algorithm>
#include <cstring>
#include <limits>

#if (WASMEDGE_OS_LINUX && !defined(__ARM_EABI__)) || WASMEDGE_OS_MACOS
extern "C" {
extern void __register_frame(void *);
extern void __deregister_frame(void *);
}
#endif

using namespace std::literals;

namespace {
uint64_t roundDownPageBoundary(const uint64_t Value,
                               const uint64_t PageSize) noexcept {
  return Value & ~(PageSize - 1);
}
uint64_t roundUpPageBoundary(const uint64_t Value,
                             const uint64_t PageSize) noexcept {
  return roundDownPageBoundary(Value + PageSize - 1, PageSize);
}
} // namespace

namespace WasmEdge::Loader {

std::optional<std::vector<size_t>>
findEHFrameFDEs(Span<const uint8_t> Section) noexcept {
  constexpr size_t FieldSize = 4;
  constexpr uint32_t ExtendedLengthMarker = UINT32_MAX;
  std::vector<size_t> Records;
  size_t Offset = 0;
  while (Offset < Section.size()) {
    const size_t Remaining = Section.size() - Offset;
    uint32_t Length = 0;
    if (Remaining >= FieldSize) {
      std::memcpy(&Length, Section.data() + Offset, FieldSize);
    }
    if (Remaining < FieldSize || Length == 0) {
      if (std::any_of(Section.begin() + Offset, Section.end(),
                      [](uint8_t Value) { return Value != 0; })) {
        return std::nullopt;
      }
      break;
    }
    if (Length == ExtendedLengthMarker || Length < FieldSize ||
        Length > Remaining - FieldSize) {
      return std::nullopt;
    }
    uint32_t CIEPointer = 0;
    std::memcpy(&CIEPointer, Section.data() + Offset + FieldSize, FieldSize);
    if (CIEPointer != 0) {
      Records.push_back(Offset);
    }
    Offset += FieldSize + Length;
  }
  return Records;
}

Expect<void> AOTSection::load(const AST::AOTSection &AOTSec) noexcept {
  unload();
  const uint64_t PageSize = Allocator::chunk_page_size();
  for (const auto &Section : AOTSec.getSections()) {
    const auto Offset = std::get<1>(Section);
    const auto Size = std::get<2>(Section);
    if (Size > std::numeric_limits<size_t>::max() - PageSize ||
        Offset > std::numeric_limits<size_t>::max() - PageSize - Size) {
      spdlog::error(ErrCode::Value::IntegerTooLarge);
      spdlog::error("    AOT section exceeds the address space."sv);
      return Unexpect(ErrCode::Value::IntegerTooLarge);
    }
    BinarySize = std::max(BinarySize, Offset + Size);
  }
  BinarySize = roundUpPageBoundary(BinarySize, PageSize);

  // The symbol addresses are offsets into the binary about to be mapped, and
  // are dereferenced without any further check, so reject the out-of-range
  // ones here.
  if (!checkAccessBound(AOTSec.getIntrinsicsAddress(),
                        sizeof(const IntrinsicsTable *))) {
    spdlog::error(ErrCode::Value::IntegerTooLarge);
    spdlog::error("    AOT intrinsics address out of range."sv);
    return Unexpect(ErrCode::Value::IntegerTooLarge);
  }
  for (const auto Address : AOTSec.getTypesAddress()) {
    if (!checkAccessBound(Address, 1)) {
      spdlog::error(ErrCode::Value::IntegerTooLarge);
      spdlog::error("    AOT type address out of range."sv);
      return Unexpect(ErrCode::Value::IntegerTooLarge);
    }
  }
  for (const auto Address : AOTSec.getCodesAddress()) {
    if (!checkAccessBound(Address, 1)) {
      spdlog::error(ErrCode::Value::IntegerTooLarge);
      spdlog::error("    AOT code address out of range."sv);
      return Unexpect(ErrCode::Value::IntegerTooLarge);
    }
  }

  Binary = Allocator::allocate_chunk(BinarySize);
  if (unlikely(!Binary)) {
    spdlog::error(ErrCode::Value::MemoryOutOfBounds);
    return Unexpect(ErrCode::Value::MemoryOutOfBounds);
  }

  std::vector<std::pair<uint8_t *, uint64_t>> ExecutableRanges;
  for (const auto &Section : AOTSec.getSections()) {
    const auto Offset = std::get<1>(Section);
    const auto Size = std::get<2>(Section);
    const auto &Content = std::get<3>(Section);
    if (Size > BinarySize || Offset > BinarySize ||
        Offset + Size > BinarySize || Content.size() > Size) {
      unload();
      return Unexpect(ErrCode::Value::IntegerTooLarge);
    }
    std::copy(Content.begin(), Content.end(), Binary + Offset);
    switch (static_cast<AOT::SectionKind>(std::get<0>(Section))) {
    case AOT::SectionKind::Text: {
      const auto O = roundDownPageBoundary(Offset, PageSize);
      const auto S = roundUpPageBoundary(Size + (Offset - O), PageSize);
      ExecutableRanges.emplace_back(Binary + O, S);
      break;
    }
    case AOT::SectionKind::Data:
      break;
    case AOT::SectionKind::BSS:
      break;
#if WASMEDGE_OS_LINUX
    case AOT::SectionKind::Unwind: // EHFrame
      EHFrameAddress = reinterpret_cast<void *>(Binary + Offset);
      break;
#elif WASMEDGE_OS_MACOS
    case AOT::SectionKind::Unwind: { // EHFrame
      auto Records =
          findEHFrameFDEs(Span<const uint8_t>(Binary + Offset, Size));
      if (!Records) {
        spdlog::error(ErrCode::Value::IllegalGrammar);
        spdlog::error("    AOT eh_frame record out of range."sv);
        unload();
        return Unexpect(ErrCode::Value::IllegalGrammar);
      }
      EHFrameRecords.clear();
      EHFrameRecords.reserve(Records->size());
      for (const auto Record : *Records) {
        EHFrameRecords.push_back(Binary + Offset + Record);
      }
      break;
    }
#elif WASMEDGE_OS_WINDOWS
    case AOT::SectionKind::Unwind: // PData
      PDataAddress = reinterpret_cast<void *>(Binary + Offset);
      PDataSize =
          static_cast<uint32_t>(Size / sizeof(winapi::RUNTIME_FUNCTION_));
      break;
#endif
    default:
      unload();
      return Unexpect(ErrCode::Value::IntegerTooLarge);
    }
  }

  for (const auto &[Pointer, Size] : ExecutableRanges) {
    if (!Allocator::set_chunk_executable(Pointer, Size)) {
      spdlog::error(ErrCode::Value::MemoryOutOfBounds);
      spdlog::error("    set_chunk_executable failed:{}"sv,
                    std::strerror(errno));
      unload();
      return Unexpect(ErrCode::Value::MemoryOutOfBounds);
    }
  }

  IntrinsicsAddress = AOTSec.getIntrinsicsAddress();
  TypesAddress = AOTSec.getTypesAddress();
  CodesAddress = AOTSec.getCodesAddress();

  Registered = true;
#if WASMEDGE_OS_LINUX
#if !defined(__ARM_EABI__)
  if (EHFrameAddress) {
    __register_frame(EHFrameAddress);
  }
#endif
#elif WASMEDGE_OS_MACOS
  for (auto *const Record : EHFrameRecords) {
    __register_frame(Record);
  }
#elif WASMEDGE_OS_WINDOWS
  if (PDataSize != 0) {
    winapi::RtlAddFunctionTable(
        static_cast<winapi::PRUNTIME_FUNCTION_>(PDataAddress), PDataSize,
        reinterpret_cast<winapi::ULONG_PTR_>(Binary));
  }
#endif

  return {};
}

void AOTSection::unload() noexcept {
  if (Binary) {
#if WASMEDGE_OS_LINUX
#if !defined(__ARM_EABI__)
    if (Registered && EHFrameAddress) {
      __deregister_frame(EHFrameAddress);
    }
#endif
#elif WASMEDGE_OS_MACOS
    if (Registered) {
      for (auto *const Record : EHFrameRecords) {
        __deregister_frame(Record);
      }
    }
#elif WASMEDGE_OS_WINDOWS
    if (Registered && PDataSize != 0) {
      winapi::RtlDeleteFunctionTable(
          static_cast<winapi::PRUNTIME_FUNCTION_>(PDataAddress));
    }
#endif
    Allocator::set_chunk_readable_writable(Binary, BinarySize);
    Allocator::release_chunk(Binary, BinarySize);
    Binary = nullptr;
  }
  BinarySize = 0;
  Registered = false;
#if WASMEDGE_OS_LINUX
  EHFrameAddress = nullptr;
#elif WASMEDGE_OS_MACOS
  EHFrameRecords.clear();
#elif WASMEDGE_OS_WINDOWS
  PDataAddress = nullptr;
  PDataSize = 0;
#endif
}

} // namespace WasmEdge::Loader
