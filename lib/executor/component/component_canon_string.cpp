// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"

#include "common/spdlog.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {

namespace {
/// Append the UTF-8 encoding of a scalar value.
void appendUTF8(std::string &Out, uint32_t CodePoint) {
  if (CodePoint < 0x80) {
    Out.push_back(static_cast<char>(CodePoint));
  } else if (CodePoint < 0x800) {
    Out.push_back(static_cast<char>(0xC0 | (CodePoint >> 6)));
    Out.push_back(static_cast<char>(0x80 | (CodePoint & 0x3F)));
  } else if (CodePoint < 0x10000) {
    Out.push_back(static_cast<char>(0xE0 | (CodePoint >> 12)));
    Out.push_back(static_cast<char>(0x80 | ((CodePoint >> 6) & 0x3F)));
    Out.push_back(static_cast<char>(0x80 | (CodePoint & 0x3F)));
  } else {
    Out.push_back(static_cast<char>(0xF0 | (CodePoint >> 18)));
    Out.push_back(static_cast<char>(0x80 | ((CodePoint >> 12) & 0x3F)));
    Out.push_back(static_cast<char>(0x80 | ((CodePoint >> 6) & 0x3F)));
    Out.push_back(static_cast<char>(0x80 | (CodePoint & 0x3F)));
  }
}

/// Decode the scalar value at Pos of a UTF-8 string and move Pos past it.
/// Error logging needs to be handled by the caller.
Expect<uint32_t> decodeUTF8(Span<const uint8_t> Bytes, size_t &Pos) {
  const uint8_t FirstByte = Bytes[Pos];
  uint32_t CodePoint = 0;
  size_t Len = 0;
  if (FirstByte < 0x80) {
    CodePoint = FirstByte;
    Len = 1;
  } else if ((FirstByte & 0xE0) == 0xC0) {
    CodePoint = FirstByte & 0x1F;
    Len = 2;
  } else if ((FirstByte & 0xF0) == 0xE0) {
    CodePoint = FirstByte & 0x0F;
    Len = 3;
  } else if ((FirstByte & 0xF8) == 0xF0) {
    CodePoint = FirstByte & 0x07;
    Len = 4;
  } else {
    return Unexpect(ErrCode::Value::ComponentUTF8Invalid);
  }
  if (Pos + Len > Bytes.size()) {
    return Unexpect(ErrCode::Value::ComponentUTF8Incomplete);
  }
  for (size_t J = 1; J < Len; ++J) {
    const uint8_t Byte = Bytes[Pos + J];
    if ((Byte & 0xC0) != 0x80) {
      return Unexpect(ErrCode::Value::ComponentUTF8Invalid);
    }
    CodePoint = (CodePoint << 6) | (Byte & 0x3F);
  }
  // Overlong forms, surrogates, and values past the scalar range.
  const bool Overlong = (Len == 2 && CodePoint < 0x80) ||
                        (Len == 3 && CodePoint < 0x800) ||
                        (Len == 4 && CodePoint < 0x10000);
  const bool Surrogate = CodePoint >= 0xD800 && CodePoint <= 0xDFFF;
  if (Overlong || Surrogate || CodePoint >= 0x110000) {
    return Unexpect(ErrCode::Value::ComponentUTF8Invalid);
  }
  Pos += Len;
  return CodePoint;
}

/// Decode the scalar values of a UTF-8 string.
Expect<std::vector<uint32_t>> decodeUTF8(Span<const uint8_t> Bytes) {
  std::vector<uint32_t> Out;
  size_t Pos = 0;
  while (Pos < Bytes.size()) {
    EXPECTED_TRY(const uint32_t CodePoint,
                 decodeUTF8(Bytes, Pos).map_error([](auto E) {
                   spdlog::error(E);
                   return E;
                 }));
    Out.push_back(CodePoint);
  }
  return Out;
}

/// Decode UTF-16 code units to scalar values.
Expect<std::vector<uint32_t>> decodeUTF16(Span<const uint8_t> Bytes) {
  std::vector<uint32_t> Out;
  const size_t Units = Bytes.size() / 2;
  for (size_t I = 0; I < Units; ++I) {
    const uint32_t CodeUnit = static_cast<uint32_t>(Bytes[2 * I]) |
                              (static_cast<uint32_t>(Bytes[2 * I + 1]) << 8);
    if (CodeUnit >= 0xD800 && CodeUnit <= 0xDBFF) {
      if (I + 1 < Units) {
        const uint32_t Low = static_cast<uint32_t>(Bytes[2 * I + 2]) |
                             (static_cast<uint32_t>(Bytes[2 * I + 3]) << 8);
        if (Low >= 0xDC00 && Low <= 0xDFFF) {
          Out.push_back(0x10000 + ((CodeUnit - 0xD800) << 10) + (Low - 0xDC00));
          I += 1;
          continue;
        }
      }
      spdlog::error(ErrCode::Value::ComponentUTF16Invalid);
      return Unexpect(ErrCode::Value::ComponentUTF16Invalid);
    }
    if (CodeUnit >= 0xDC00 && CodeUnit <= 0xDFFF) {
      spdlog::error(ErrCode::Value::ComponentUTF16Invalid);
      return Unexpect(ErrCode::Value::ComponentUTF16Invalid);
    }
    Out.push_back(CodeUnit);
  }
  return Out;
}

/// Encode scalar values as UTF-16 code units, little-endian.
std::vector<uint8_t> encodeUTF16(Span<const uint32_t> CodePoints) {
  std::vector<uint8_t> Out;
  auto PushUnit = [&Out](uint32_t CodeUnit) {
    Out.push_back(static_cast<uint8_t>(CodeUnit & 0xFF));
    Out.push_back(static_cast<uint8_t>((CodeUnit >> 8) & 0xFF));
  };
  for (const uint32_t CodePoint : CodePoints) {
    if (CodePoint >= 0x10000) {
      const uint32_t Offset = CodePoint - 0x10000;
      PushUnit(0xD800 | (Offset >> 10));
      PushUnit(0xDC00 | (Offset & 0x3FF));
    } else {
      PushUnit(CodePoint);
    }
  }
  return Out;
}
} // namespace

// Load a string. See "include/executor/component/executor.h".
Expect<ComponentValVariant>
ComponentExecutor::loadString(const Runtime::Component::CanonOptions &Opts,
                              uint64_t Ptr, uint64_t PackedLen) {
  Runtime::Component::StringEncoding Encoding = Opts.Encoding;
  uint64_t Units = PackedLen;
  if (Encoding == Runtime::Component::StringEncoding::Latin1UTF16) {
    // The tag bit of the packed length selects UTF-16 over Latin-1.
    if ((PackedLen & Opts.getUTF16Tag()) != 0) {
      Encoding = Runtime::Component::StringEncoding::UTF16;
      Units = PackedLen & ~Opts.getUTF16Tag();
    }
  }
  // A lift across the host boundary faults on the pointer and length; one
  // between two components faults on the content.
  const Runtime::Component::Task *T = getCurrentTask(*Opts.Inst);
  const bool HostBoundary = T == nullptr || T->isHost() || !T->hasGuestCaller();
  const ErrCode::Value OOB = HostBoundary
                                 ? ErrCode::Value::ComponentStrPtrLenOOB
                                 : ErrCode::Value::ComponentStrOOB;
  switch (Encoding) {
  case Runtime::Component::StringEncoding::UTF8: {
    EXPECTED_TRY(checkByteLength(Units, 1));
    EXPECTED_TRY(checkInBounds(Opts, Ptr, Units, OOB));
    EXPECTED_TRY(auto Bytes, Opts.Mem->getBytes(Ptr, Units));
    // Well-formed UTF-8 is the string as it is.
    size_t Pos = 0;
    while (Pos < Bytes.size()) {
      EXPECTED_TRY(decodeUTF8(Bytes, Pos).map_error([](auto E) {
        spdlog::error(E);
        return E;
      }));
    }
    return ComponentValVariant{std::string(Bytes.begin(), Bytes.end())};
  }
  case Runtime::Component::StringEncoding::UTF16: {
    EXPECTED_TRY(checkByteLength(Units, 2));
    EXPECTED_TRY(checkAligned(Ptr, 2));
    EXPECTED_TRY(checkInBounds(Opts, Ptr, Units * 2, OOB));
    EXPECTED_TRY(auto Bytes, Opts.Mem->getBytes(Ptr, Units * 2));
    EXPECTED_TRY(auto CodePoints, decodeUTF16(Bytes));
    std::string Out;
    for (const uint32_t CodePoint : CodePoints) {
      appendUTF8(Out, CodePoint);
    }
    return makeComponentVal(
        StringVal{std::move(Out),
                  Opts.Encoding == Runtime::Component::StringEncoding::UTF16
                      ? StringHint::UTF16
                      : StringHint::DynamicUTF16});
  }
  case Runtime::Component::StringEncoding::Latin1UTF16: {
    EXPECTED_TRY(checkByteLength(Units, 1));
    EXPECTED_TRY(checkAligned(Ptr, 2));
    EXPECTED_TRY(checkInBounds(Opts, Ptr, Units, OOB));
    EXPECTED_TRY(auto Bytes, Opts.Mem->getBytes(Ptr, Units));
    std::string Out;
    for (const uint8_t Byte : Bytes) {
      appendUTF8(Out, Byte);
    }
    return makeComponentVal(
        StringVal{std::move(Out), StringHint::DynamicLatin1});
  }
  default:
    assumingUnreachable();
  }
}

// Store a string. See "include/executor/component/executor.h".
Expect<std::pair<uint64_t, uint64_t>>
ComponentExecutor::storeString(const Runtime::Component::CanonOptions &Opts,
                               const ComponentValVariant &Val) {
  // A host string is UTF-8; a string in transfer names its source.
  std::string_view Str;
  bool IsUTF8 = true;
  bool IsUTF16 = false;
  bool IsTagged = false;
  if (const auto *HostStr = std::get_if<std::string>(&Val)) {
    Str = *HostStr;
  } else {
    const auto &Transfer = getComponentVal<StringVal>(Val);
    Str = Transfer.Str;
    IsUTF8 = false;
    IsUTF16 = Transfer.Hint != StringHint::DynamicLatin1;
    IsTagged = Transfer.Hint == StringHint::DynamicUTF16;
  }
  const Span<const uint8_t> Src(reinterpret_cast<const uint8_t *>(Str.data()),
                                Str.size());
  auto Write = [&Opts](Span<const uint8_t> Bytes, uint64_t At) {
    return Opts.Mem->setBytes(Bytes, At, 0, Bytes.size());
  };
  if (IsUTF8 && Opts.Encoding == Runtime::Component::StringEncoding::UTF8) {
    // A copy of the code units.
    EXPECTED_TRY(const uint64_t Ptr, invokeRealloc(Opts, 0, 0, 1, Src.size()));
    EXPECTED_TRY(Write(Src, Ptr));
    return std::make_pair(Ptr, static_cast<uint64_t>(Src.size()));
  }
  EXPECTED_TRY(auto CodePoints, decodeUTF8(Src));
  const auto Encoded = encodeUTF16(CodePoints);
  // The code units of the source: UTF-8 bytes, UTF-16 units, or Latin-1 bytes.
  uint64_t SrcUnits = CodePoints.size();
  if (IsUTF8) {
    SrcUnits = Src.size();
  } else if (IsUTF16) {
    SrcUnits = Encoded.size() / 2;
  }
  switch (Opts.Encoding) {
  case Runtime::Component::StringEncoding::UTF8: {
    // One byte per code unit while the scalars are ASCII, else the worst case
    // from the first other scalar on, shrunk afterwards.
    const uint64_t MaxBytes = SrcUnits * (IsUTF16 ? 3 : 2);
    EXPECTED_TRY(uint64_t Ptr, invokeRealloc(Opts, 0, 0, 1, SrcUnits));
    size_t Ascii = 0;
    while (Ascii < CodePoints.size() && CodePoints[Ascii] < 0x80) {
      ++Ascii;
    }
    EXPECTED_TRY(Write(Src.first(Ascii), Ptr));
    if (Ascii == CodePoints.size()) {
      return std::make_pair(Ptr, SrcUnits);
    }
    // Only the rest is written: the bytes before it are those realloc kept.
    EXPECTED_TRY(Ptr, invokeRealloc(Opts, Ptr, SrcUnits, 1, MaxBytes));
    EXPECTED_TRY(Write(Src.subspan(Ascii), Ptr + Ascii));
    if (Src.size() < MaxBytes) {
      EXPECTED_TRY(Ptr, invokeRealloc(Opts, Ptr, MaxBytes, 1, Src.size()));
    }
    return std::make_pair(Ptr, static_cast<uint64_t>(Src.size()));
  }
  case Runtime::Component::StringEncoding::UTF16: {
    if (!IsUTF8) {
      // A copy of UTF-16, or Latin-1 at two bytes per code unit.
      EXPECTED_TRY(const uint64_t Ptr,
                   invokeRealloc(Opts, 0, 0, 2, Encoded.size()));
      EXPECTED_TRY(Write(Encoded, Ptr));
      return std::make_pair(Ptr, SrcUnits);
    }
    // Worst case two code units per source byte, shrunk afterwards.
    const uint64_t MaxBytes = 2 * SrcUnits;
    EXPECTED_TRY(uint64_t Ptr, invokeRealloc(Opts, 0, 0, 2, MaxBytes));
    EXPECTED_TRY(Write(Encoded, Ptr));
    if (Encoded.size() < MaxBytes) {
      EXPECTED_TRY(Ptr, invokeRealloc(Opts, Ptr, MaxBytes, 2, Encoded.size()));
    }
    return std::make_pair(Ptr, static_cast<uint64_t>(Encoded.size() / 2));
  }
  case Runtime::Component::StringEncoding::Latin1UTF16: {
    // The scalars that lead the string and fit one byte of Latin-1.
    std::vector<uint8_t> Latin1;
    for (const uint32_t CodePoint : CodePoints) {
      if (CodePoint >= 0x100) {
        break;
      }
      Latin1.push_back(static_cast<uint8_t>(CodePoint));
    }
    const bool FitsLatin1 = Latin1.size() == CodePoints.size();
    if (!IsUTF8 && !IsUTF16) {
      // A copy of Latin-1.
      EXPECTED_TRY(const uint64_t Ptr, invokeRealloc(Opts, 0, 0, 2, SrcUnits));
      EXPECTED_TRY(Write(Latin1, Ptr));
      return std::make_pair(Ptr, SrcUnits);
    }
    if (IsTagged) {
      // UTF-16 its producer chose over Latin-1: stored as it is, and shrunk to
      // Latin-1 when every scalar fits.
      EXPECTED_TRY(uint64_t Ptr, invokeRealloc(Opts, 0, 0, 2, Encoded.size()));
      EXPECTED_TRY(Write(Encoded, Ptr));
      if (!FitsLatin1) {
        return std::make_pair(Ptr, SrcUnits | Opts.getUTF16Tag());
      }
      EXPECTED_TRY(Write(Latin1, Ptr));
      EXPECTED_TRY(Ptr,
                   invokeRealloc(Opts, Ptr, Encoded.size(), 2, Latin1.size()));
      return std::make_pair(Ptr, static_cast<uint64_t>(Latin1.size()));
    }
    // Latin-1 at one byte per code unit while every scalar fits, shrunk
    // afterwards; else UTF-16 with the tagged length, from the worst case.
    EXPECTED_TRY(uint64_t Ptr, invokeRealloc(Opts, 0, 0, 2, SrcUnits));
    EXPECTED_TRY(Write(Latin1, Ptr));
    if (FitsLatin1) {
      if (Latin1.size() < SrcUnits) {
        EXPECTED_TRY(Ptr, invokeRealloc(Opts, Ptr, SrcUnits, 2, Latin1.size()));
      }
      return std::make_pair(Ptr, static_cast<uint64_t>(Latin1.size()));
    }
    // The Latin-1 bytes realloc kept widen in place and the rest follows.
    const uint64_t MaxBytes = 2 * SrcUnits;
    EXPECTED_TRY(Ptr, invokeRealloc(Opts, Ptr, SrcUnits, 2, MaxBytes));
    EXPECTED_TRY(auto Kept, Opts.Mem->getBytes(Ptr, Latin1.size()));
    std::vector<uint8_t> Wide(2 * Latin1.size(), 0);
    for (size_t I = 0; I < Latin1.size(); ++I) {
      Wide[2 * I] = Kept[I];
    }
    EXPECTED_TRY(Write(Wide, Ptr));
    EXPECTED_TRY(Write(Span<const uint8_t>{Encoded}.subspan(Wide.size()),
                       Ptr + Wide.size()));
    if (Encoded.size() < MaxBytes) {
      EXPECTED_TRY(Ptr, invokeRealloc(Opts, Ptr, MaxBytes, 2, Encoded.size()));
    }
    return std::make_pair(Ptr, (Encoded.size() / 2) | Opts.getUTF16Tag());
  }
  default:
    assumingUnreachable();
  }
}

// Whether a string is well-formed UTF-8. See
// "include/executor/component/executor.h".
bool ComponentExecutor::isValidUTF8(std::string_view Str) const noexcept {
  const Span<const uint8_t> Bytes(reinterpret_cast<const uint8_t *>(Str.data()),
                                  Str.size());
  size_t Pos = 0;
  while (Pos < Bytes.size()) {
    if (!decodeUTF8(Bytes, Pos)) {
      return false;
    }
  }
  return true;
}

} // namespace Executor
} // namespace WasmEdge
