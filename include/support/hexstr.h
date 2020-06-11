// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "support/span.h"

#include <cstdio>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace SSVM {
namespace Support {

inline unsigned char convertCharToHex(const char C) {
  if (C >= '0' && C <= '9') {
    return C - '0';
  }
  if (C >= 'a' && C <= 'f') {
    return (C - 'a') + 10;
  }
  if (C >= 'A' && C <= 'F') {
    return (C - 'A') + 10;
  }
  return 0U;
}

inline void convertBytesToHexStr(Span<const unsigned char> Src,
                                 std::string &Dst, uint32_t Padding = 0,
                                 bool IsLittleEndian = false) {
  Dst.clear();
  char Buf[3] = {0};
  if (IsLittleEndian) {
    for (auto It = Src.rbegin(); It != Src.rend(); It++) {
      std::snprintf(Buf, 3, "%02x", *It);
      Dst += Buf;
    }
  } else {
    for (auto It = Src.begin(); It != Src.end(); It++) {
      std::snprintf(Buf, 3, "%02x", *It);
      Dst += Buf;
    }
  }
  if (Dst.length() < Padding) {
    Dst = std::string(Padding - Dst.length(), '0').append(Dst);
  }
}

inline void convertValVecToHexStr(Span<const unsigned char> Src,
                                  std::string &Dst, uint32_t Padding = 0) {
  convertBytesToHexStr(Src, Dst, Padding, true);
}

inline void convertHexStrToBytes(std::string_view Src,
                                 std::vector<unsigned char> &Dst,
                                 uint32_t Padding = 2,
                                 bool IsLittleEndian = false) {
  if (Padding & 0x01U) {
    Padding++;
  }
  Dst.clear();
  if (Src.length() == 0) {
    return;
  }
  std::string S(Src);
  if (S.length() < Padding) {
    S = std::string(Padding - S.length(), '0').append(S);
  }
  if (S.length() & 0x01U) {
    S = '0' + S;
  }
  if (IsLittleEndian) {
    for (auto It = S.crbegin(); It != S.crend(); It += 2) {
      char CL = *It;
      char CH = *(It + 1);
      Dst.push_back(convertCharToHex(CL) + (convertCharToHex(CH) << 4));
    }
  } else {
    for (auto It = S.cbegin(); It != S.cend(); It += 2) {
      char CH = *It;
      char CL = *(It + 1);
      Dst.push_back(convertCharToHex(CL) + (convertCharToHex(CH) << 4));
    }
  }
}

inline void convertHexStrToValVec(std::string_view Src,
                                  std::vector<unsigned char> &Dst,
                                  uint32_t Padding = 2) {
  convertHexStrToBytes(Src, Dst, Padding);
}

inline std::string convertUIntToHexStr(uint64_t Num, uint32_t MinLen = 8) {
  char Buf[16] = {0}, Str[32] = {0};
  MinLen = (MinLen > 16 ? 16 : MinLen);
  sprintf(Buf, "0x%%0%dllx", MinLen);
  sprintf(Str, Buf, Num);
  return Str;
}

} // namespace Support
} // namespace SSVM
