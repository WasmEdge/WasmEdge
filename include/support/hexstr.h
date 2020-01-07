// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <iterator>
#include <stdio.h>
#include <string>
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

inline void convertBytesToHexStr(const std::vector<unsigned char> &Src,
                                 std::string &Dst, uint32_t Padding = 0,
                                 bool IsLittleEndian = false) {
  Dst.clear();
  char Buf[3] = {0};
  if (IsLittleEndian) {
    for (auto It = Src.crbegin(); It != Src.crend(); It++) {
      snprintf(Buf, 3, "%02x", *It);
      Dst += Buf;
    }
  } else {
    for (auto It = Src.cbegin(); It != Src.cend(); It++) {
      snprintf(Buf, 3, "%02x", *It);
      Dst += Buf;
    }
  }
  if (Dst.length() < Padding) {
    Dst = std::string(Padding - Dst.length(), '0').append(Dst);
  }
}

inline void convertValVecToHexStr(const std::vector<unsigned char> &Src,
                                  std::string &Dst, uint32_t Padding = 0) {
  convertBytesToHexStr(Src, Dst, Padding, true);
}

inline void convertHexStrToBytes(const std::string &Src,
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
  std::string S = Src;
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

inline void convertHexStrToValVec(const std::string &Src,
                                  std::vector<unsigned char> &Dst,
                                  unsigned int Padding = 2) {
  convertHexStrToBytes(Src, Dst, Padding);
}

} // namespace Support
} // namespace SSVM
