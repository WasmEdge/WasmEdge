// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/canonical_abi.h"

#include "common/component_variant.h"
#include "common/errinfo.h"
#include "common/spdlog.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {
namespace Component {

namespace {

// The host holds component strings as UTF-8; getEncoding() is the guest form.

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

// Decode one UTF-8 scalar at S[I], advancing I; traps if malformed.
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
    EXPECTED_TRY(LiftLowerContext::trapDataInvalid(
        "invalid UTF-8 lead byte", ErrCode::Value::ComponentUTF8Invalid));
  }
  if (I + Len > N) {
    EXPECTED_TRY(LiftLowerContext::trapDataInvalid(
        "truncated UTF-8 sequence", ErrCode::Value::ComponentUTF8Incomplete));
  }
  for (size_t K = 1; K < Len; ++K) {
    const uint8_t B = static_cast<uint8_t>(S[I + K]);
    if ((B & 0xC0u) != 0x80u) {
      EXPECTED_TRY(LiftLowerContext::trapDataInvalid(
          "invalid UTF-8 continuation byte",
          ErrCode::Value::ComponentUTF8Invalid));
    }
    CP = (CP << 6) | (B & 0x3Fu);
  }
  if (CP < Min || CP > 0x10FFFFu || (CP >= 0xD800u && CP <= 0xDFFFu)) {
    EXPECTED_TRY(LiftLowerContext::trapDataInvalid(
        "invalid UTF-8 scalar value", ErrCode::Value::ComponentUTF8Invalid));
  }
  I += Len;
  return CP;
}

// Validate S as UTF-8 without allocating, so the raw view can travel.
Expect<void> validateUtf8(std::string_view S) noexcept {
  for (size_t I = 0; I < S.size();) {
    EXPECTED_TRY(auto CP, decodeUtf8Scalar(S, I));
    static_cast<void>(CP);
  }
  return {};
}

// Decode a UTF-8 host string into scalar values, validating as we go.
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
  // A UTF-16 code unit is at most 3 UTF-8 bytes, so this fits.
  Out.reserve(N / 2 * 3);
  for (size_t I = 0; I + 1 < N; I += 2) {
    uint32_t U =
        static_cast<uint32_t>(static_cast<uint8_t>(Bytes[I])) |
        (static_cast<uint32_t>(static_cast<uint8_t>(Bytes[I + 1])) << 8);
    if (U >= 0xD800u && U <= 0xDBFFu) {
      if (I + 3 >= N) {
        EXPECTED_TRY(LiftLowerContext::trapDataInvalid(
            "unpaired UTF-16 high surrogate"));
      }
      const uint32_t L =
          static_cast<uint32_t>(static_cast<uint8_t>(Bytes[I + 2])) |
          (static_cast<uint32_t>(static_cast<uint8_t>(Bytes[I + 3])) << 8);
      if (L < 0xDC00u || L > 0xDFFFu) {
        EXPECTED_TRY(
            LiftLowerContext::trapDataInvalid("invalid UTF-16 low surrogate"));
      }
      U = 0x10000u + ((U - 0xD800u) << 10) + (L - 0xDC00u);
      I += 2;
    } else if (U >= 0xDC00u && U <= 0xDFFFu) {
      EXPECTED_TRY(
          LiftLowerContext::trapDataInvalid("unpaired UTF-16 low surrogate"));
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
  auto Push16 = [&](uint32_t U) {
    Out.push_back(static_cast<Byte>(U & 0xFFu));
    Out.push_back(static_cast<Byte>((U >> 8) & 0xFFu));
  };
  for (uint32_t CP : CPs) {
    if (CP < 0x10000u) {
      Push16(CP);
    } else {
      const uint32_t V = CP - 0x10000u;
      Push16(0xD800u + (V >> 10));
      Push16(0xDC00u + (V & 0x3FFu));
    }
  }
  return Out;
}
} // namespace

// Decode tagged_code_units of guest bytes at Begin into UTF-8.
Expect<std::string>
LiftLowerContext::decodeString(uint64_t Begin,
                               uint64_t TaggedCodeUnits) const noexcept {
  enum class WireEnc { Utf8, Utf16, Latin1 } W = WireEnc::Utf8;
  uint32_t Alignment = 1;
  uint64_t ByteLen64 = 0;
  switch (getEncoding()) {
  case Runtime::Component::StringEncoding::UTF8:
    W = WireEnc::Utf8;
    Alignment = 1;
    ByteLen64 = TaggedCodeUnits;
    break;
  case Runtime::Component::StringEncoding::UTF16:
    W = WireEnc::Utf16;
    Alignment = 2;
    ByteLen64 = 2ull * TaggedCodeUnits;
    break;
  case Runtime::Component::StringEncoding::Latin1UTF16:
    Alignment = 2;
    if ((TaggedCodeUnits & Utf16LengthTag) != 0u) {
      W = WireEnc::Utf16;
      ByteLen64 = 2ull * (TaggedCodeUnits ^ Utf16LengthTag);
    } else {
      W = WireEnc::Latin1;
      ByteLen64 = TaggedCodeUnits;
    }
    break;
  }
  if (ByteLen64 > static_cast<uint64_t>(MaxCanonByteLength)) {
    EXPECTED_TRY(trapDataInvalid("string byte length exceeds MAX"));
  }
  const uint32_t ByteLen = static_cast<uint32_t>(ByteLen64);
  if (Begin != alignTo(Begin, Alignment)) {
    EXPECTED_TRY(trapDataInvalid("unaligned pointer for string",
                                 ErrCode::Value::ComponentPtrUnaligned));
  }
  if (!getMemory()->checkAccessBound(Begin, ByteLen)) {
    // A cross-component adapter and the host boundary differ in diagnostics.
    const auto Code = isCrossComponent()
                          ? ErrCode::Value::ComponentStrOOB
                          : ErrCode::Value::ComponentStrPtrLenOOB;
    spdlog::error(Code);
    spdlog::error("    canonical ABI: string at 0x{:x} len={} out of bounds"sv,
                  Begin, ByteLen);
    return Unexpect(Code);
  }
  auto SV = getMemory()->getStringView(Begin, ByteLen);
  switch (W) {
  case WireEnc::Utf8: {
    // Validate before handing bytes to the host, which expects UTF-8.
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

// Encode into a fresh guest buffer, returning (begin, tagged_code_units).
Expect<std::pair<uint64_t, uint64_t>>
LiftLowerContext::encodeString(const std::string &S) const noexcept {
  // Realloc a buffer, bounds-check it, then copy `Bytes` in.
  auto WriteBuf = [&](Span<const Byte> Bytes,
                      uint32_t Align) -> Expect<uint32_t> {
    const uint32_t Len = static_cast<uint32_t>(Bytes.size());
    // Realloc runs even for an empty payload, so a bad one traps.
    EXPECTED_TRY(auto Begin, callRealloc(0u, 0u, Align, Len));
    if (Begin != alignTo(Begin, Align)) {
      EXPECTED_TRY(trapDataInvalid("unaligned pointer for string buffer",
                                   ErrCode::Value::ComponentPtrUnaligned));
    }
    if (!getMemory()->checkAccessBound(Begin, Len)) {
      EXPECTED_TRY(trapMemoryOOB(Begin, Len));
    }
    EXPECTED_TRY(getMemory()->setBytes(Bytes, Begin, 0u, Len));
    return Begin;
  };

  // UTF-16-LE into a 2-aligned buffer; Tag sets the utf16 marker.
  auto WriteUtf16 = [&](const std::vector<uint32_t> &CPs,
                        bool Tag) -> Expect<std::pair<uint32_t, uint32_t>> {
    auto Bytes = codePointsToUtf16le(CPs);
    const uint32_t Units = static_cast<uint32_t>(Bytes.size() / 2);
    EXPECTED_TRY(auto Begin, WriteBuf(Bytes, 2u));
    return std::make_pair(Begin, Tag ? (Units | Utf16LengthTag) : Units);
  };

  switch (getEncoding()) {
  case Runtime::Component::StringEncoding::UTF8: {
    // utf8 -> utf8 is a plain byte copy.
    Span<const Byte> Bytes{reinterpret_cast<const Byte *>(S.data()), S.size()};
    EXPECTED_TRY(auto Begin, WriteBuf(Bytes, 1u));
    return std::make_pair(Begin, static_cast<uint32_t>(S.size()));
  }
  case Runtime::Component::StringEncoding::UTF16: {
    // utf8 -> utf16: code units = bytes / 2.
    EXPECTED_TRY(auto CPs, utf8ToCodePoints(S));
    return WriteUtf16(CPs, /*Tag=*/false);
  }
  case Runtime::Component::StringEncoding::Latin1UTF16: {
    // latin1 when every scalar fits a byte, else tagged utf16.
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
      EXPECTED_TRY(auto Begin, WriteBuf(Latin1, 2u));
      return std::make_pair(Begin, static_cast<uint32_t>(CPs.size()));
    }
    return WriteUtf16(CPs, /*Tag=*/true);
  }
  }
  assumingUnreachable();
}

} // namespace Component
} // namespace Executor
} // namespace WasmEdge
