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
Expect<void>
checkStringBounds(const Runtime::Component::CanonOptions &Opts, uint64_t Ptr,
                  uint64_t ByteLen,
                  ErrCode::Value OOB = ErrCode::Value::ComponentStrOOB) {
  if (Opts.Mem == nullptr) {
    spdlog::error(ErrCode::Value::InvalidCanonOption);
    spdlog::error("    the canonical option `memory` is required"sv);
    return Unexpect(ErrCode::Value::InvalidCanonOption);
  }
  if (!Opts.Mem->checkAccessBound(Ptr, ByteLen)) {
    spdlog::error(OOB);
    return Unexpect(OOB);
  }
  return {};
}

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

/// Decode the scalar values of a UTF-8 string; nullopt on an invalid one.
Expect<std::vector<uint32_t>> decodeUTF8(Span<const uint8_t> Bytes) {
  std::vector<uint32_t> Out;
  size_t I = 0;
  while (I < Bytes.size()) {
    const uint8_t FirstByte = Bytes[I];
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
      spdlog::error(ErrCode::Value::ComponentUTF8Invalid);
      return Unexpect(ErrCode::Value::ComponentUTF8Invalid);
    }
    if (I + Len > Bytes.size()) {
      spdlog::error(ErrCode::Value::ComponentUTF8Incomplete);
      return Unexpect(ErrCode::Value::ComponentUTF8Incomplete);
    }
    for (size_t J = 1; J < Len; ++J) {
      const uint8_t Byte = Bytes[I + J];
      if ((Byte & 0xC0) != 0x80) {
        spdlog::error(ErrCode::Value::ComponentUTF8Invalid);
        return Unexpect(ErrCode::Value::ComponentUTF8Invalid);
      }
      CodePoint = (CodePoint << 6) | (Byte & 0x3F);
    }
    // Overlong forms, surrogates, and values past the scalar range.
    const bool Overlong = (Len == 2 && CodePoint < 0x80) ||
                          (Len == 3 && CodePoint < 0x800) ||
                          (Len == 4 && CodePoint < 0x10000);
    if (Overlong || !ComponentExecutor::isValidChar(CodePoint)) {
      spdlog::error(ErrCode::Value::ComponentUTF8Invalid);
      return Unexpect(ErrCode::Value::ComponentUTF8Invalid);
    }
    Out.push_back(CodePoint);
    I += Len;
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

// Whether a code point is a scalar value. See
// "include/executor/component/executor.h".
bool ComponentExecutor::isValidChar(uint32_t CodePoint) noexcept {
  return CodePoint < 0x110000 && !(CodePoint >= 0xD800 && CodePoint <= 0xDFFF);
}

// Load a string. See "include/executor/component/executor.h".
Expect<std::string>
ComponentExecutor::loadString(const Runtime::Component::CanonOptions &Opts,
                              uint64_t Ptr, uint64_t PackedLen) {
  Runtime::Component::StringEncoding Encoding = Opts.Encoding;
  uint64_t Units = PackedLen;
  if (Encoding == Runtime::Component::StringEncoding::Latin1UTF16) {
    // The tag bit of the packed length selects UTF-16 over Latin-1.
    if ((PackedLen & UTF16Tag) != 0) {
      Encoding = Runtime::Component::StringEncoding::UTF16;
      Units = PackedLen & ~UTF16Tag;
    }
  }
  // A lift across the host boundary faults on the pointer and length; one
  // between two components faults on the content.
  const Runtime::Component::Task *T = getCurrentTask();
  const bool HostBoundary = T == nullptr || T->isHost() ||
                            T->getCaller() == nullptr ||
                            T->getCaller()->isHost();
  const ErrCode::Value OOB = HostBoundary
                                 ? ErrCode::Value::ComponentStrPtrLenOOB
                                 : ErrCode::Value::ComponentStrOOB;
  switch (Encoding) {
  case Runtime::Component::StringEncoding::UTF8: {
    EXPECTED_TRY(checkByteLength(Units, 1));
    EXPECTED_TRY(checkStringBounds(Opts, Ptr, Units, OOB));
    EXPECTED_TRY(auto Bytes, Opts.Mem->getBytes(Ptr, Units));
    EXPECTED_TRY(auto CodePoints, decodeUTF8(Bytes));
    std::string Out;
    Out.reserve(Units);
    for (const uint32_t CodePoint : CodePoints) {
      appendUTF8(Out, CodePoint);
    }
    return Out;
  }
  case Runtime::Component::StringEncoding::UTF16: {
    EXPECTED_TRY(checkByteLength(Units, 2));
    EXPECTED_TRY(checkAligned(Ptr, 2));
    EXPECTED_TRY(checkStringBounds(Opts, Ptr, Units * 2, OOB));
    EXPECTED_TRY(auto Bytes, Opts.Mem->getBytes(Ptr, Units * 2));
    EXPECTED_TRY(auto CodePoints, decodeUTF16(Bytes));
    std::string Out;
    for (const uint32_t CodePoint : CodePoints) {
      appendUTF8(Out, CodePoint);
    }
    return Out;
  }
  case Runtime::Component::StringEncoding::Latin1UTF16: {
    EXPECTED_TRY(checkByteLength(Units, 1));
    EXPECTED_TRY(checkAligned(Ptr, 2));
    EXPECTED_TRY(checkStringBounds(Opts, Ptr, Units, OOB));
    EXPECTED_TRY(auto Bytes, Opts.Mem->getBytes(Ptr, Units));
    std::string Out;
    for (const uint8_t Byte : Bytes) {
      appendUTF8(Out, Byte);
    }
    return Out;
  }
  default:
    assumingUnreachable();
  }
}

// Store a string. See "include/executor/component/executor.h".
Expect<std::pair<uint64_t, uint64_t>>
ComponentExecutor::storeString(const Runtime::Component::CanonOptions &Opts,
                               std::string_view Str) {
  const Span<const uint8_t> Src(reinterpret_cast<const uint8_t *>(Str.data()),
                                Str.size());
  const uint64_t SrcUnits = Str.size();
  switch (Opts.Encoding) {
  case Runtime::Component::StringEncoding::UTF8: {
    EXPECTED_TRY(const uint64_t Ptr, invokeRealloc(Opts, 0, 0, 1, SrcUnits));
    EXPECTED_TRY(checkStringBounds(Opts, Ptr, SrcUnits));
    EXPECTED_TRY(Opts.Mem->setBytes(Src, Ptr, 0, SrcUnits));
    return std::make_pair(Ptr, SrcUnits);
  }
  case Runtime::Component::StringEncoding::UTF16: {
    // Worst case two code units per source byte, shrunk afterwards.
    EXPECTED_TRY(auto CodePoints, decodeUTF8(Src));
    const auto Encoded = encodeUTF16(CodePoints);
    const uint64_t WorstCase = 2 * SrcUnits;
    EXPECTED_TRY(uint64_t Ptr, invokeRealloc(Opts, 0, 0, 2, WorstCase));
    EXPECTED_TRY(checkStringBounds(Opts, Ptr, Encoded.size()));
    EXPECTED_TRY(Opts.Mem->setBytes(Encoded, Ptr, 0, Encoded.size()));
    if (Encoded.size() < WorstCase) {
      EXPECTED_TRY(Ptr, invokeRealloc(Opts, Ptr, WorstCase, 2, Encoded.size()));
    }
    return std::make_pair(Ptr, static_cast<uint64_t>(Encoded.size() / 2));
  }
  case Runtime::Component::StringEncoding::Latin1UTF16: {
    // Latin-1 while every scalar fits a byte, else UTF-16 with the tagged
    // length.
    EXPECTED_TRY(auto CodePoints, decodeUTF8(Src));
    EXPECTED_TRY(uint64_t Ptr, invokeRealloc(Opts, 0, 0, 2, SrcUnits));
    std::vector<uint8_t> Latin1;
    bool Fits = true;
    for (const uint32_t CodePoint : CodePoints) {
      if (CodePoint >= 0x100) {
        Fits = false;
        break;
      }
      Latin1.push_back(static_cast<uint8_t>(CodePoint));
    }
    if (Fits) {
      EXPECTED_TRY(checkStringBounds(Opts, Ptr, Latin1.size()));
      EXPECTED_TRY(Opts.Mem->setBytes(Latin1, Ptr, 0, Latin1.size()));
      if (Latin1.size() < SrcUnits) {
        EXPECTED_TRY(Ptr, invokeRealloc(Opts, Ptr, SrcUnits, 2, Latin1.size()));
      }
      return std::make_pair(Ptr, static_cast<uint64_t>(Latin1.size()));
    }
    const auto Encoded = encodeUTF16(CodePoints);
    const uint64_t WorstCase = 2 * SrcUnits;
    EXPECTED_TRY(Ptr, invokeRealloc(Opts, Ptr, SrcUnits, 2, WorstCase));
    EXPECTED_TRY(checkStringBounds(Opts, Ptr, Encoded.size()));
    EXPECTED_TRY(Opts.Mem->setBytes(Encoded, Ptr, 0, Encoded.size()));
    if (Encoded.size() < WorstCase) {
      EXPECTED_TRY(Ptr, invokeRealloc(Opts, Ptr, WorstCase, 2, Encoded.size()));
    }
    return std::make_pair(Ptr, (Encoded.size() / 2) | UTF16Tag);
  }
  default:
    assumingUnreachable();
  }
}

} // namespace Executor
} // namespace WasmEdge
