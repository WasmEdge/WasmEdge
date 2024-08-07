// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/hexstr.h"

#include <algorithm>
#include <spdlog/fmt/fmt.h>

namespace WasmEdge {

uint8_t convertCharToHex(const char C) {
  if (C >= '0' && C <= '9') {
    return static_cast<uint8_t>(C - '0') + UINT8_C(0);
  }
  if (C >= 'a' && C <= 'f') {
    return static_cast<uint8_t>(C - 'a') + UINT8_C(10);
  }
  if (C >= 'A' && C <= 'F') {
    return static_cast<uint8_t>(C - 'A') + UINT8_C(10);
  }
  return UINT8_C(0);
}

void convertBytesToHexStr(Span<const uint8_t> Src, std::string &Dst,
                          const uint32_t Padding, const bool IsLittleEndian) {
  Dst.clear();
  Dst.reserve(Src.size() * 2);
  if (IsLittleEndian) {
    for (auto It = Src.rbegin(); It != Src.rend(); It++) {
      Dst += fmt::format("{:02x}", *It);
    }
  } else {
    for (auto It = Src.begin(); It != Src.end(); It++) {
      Dst += fmt::format("{:02x}", *It);
    }
  }
  if (Dst.length() < Padding) {
    Dst = std::string(Padding - Dst.length(), '0').append(Dst);
  }
}

void convertValVecToHexStr(Span<const uint8_t> Src, std::string &Dst,
                           const uint32_t Padding) {
  convertBytesToHexStr(Src, Dst, Padding, true);
}

void convertHexStrToBytes(std::string_view Src, std::vector<uint8_t> &Dst,
                          uint32_t Padding, const bool IsLittleEndian) {
  if (Padding & 0x01U) {
    Padding++;
  }
  Dst.clear();
  if (Src.size() == 0) {
    return;
  }
  std::string S(Src);
  if (S.length() < Padding) {
    S = std::string(Padding - S.length(), '0').append(S);
  }
  if (S.length() & 0x01U) {
    S = '0' + S;
  }
  Dst.reserve(S.size() / 2);
  if (IsLittleEndian) {
    for (auto It = S.crbegin(); It != S.crend(); It += 2) {
      uint8_t CL = convertCharToHex(*It);
      uint8_t CH = convertCharToHex(*(It + 1)) * static_cast<uint8_t>(16);
      Dst.push_back(CL + CH);
    }
  } else {
    for (auto It = S.cbegin(); It != S.cend(); It += 2) {
      uint8_t CH = convertCharToHex(*It) * static_cast<uint8_t>(16);
      uint8_t CL = convertCharToHex(*(It + 1));
      Dst.push_back(CL + CH);
    }
  }
}

void convertHexStrToValVec(std::string_view Src, std::vector<uint8_t> &Dst,
                           const uint32_t Padding) {
  convertHexStrToBytes(Src, Dst, Padding);
}

std::string convertUIntToHexStr(const uint64_t Num, uint32_t MinLen) {
  return fmt::format("0x{:0{}x}", Num, std::min(MinLen, UINT32_C(16)));
}

} // namespace WasmEdge
