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

inline bool convertHexToString(std::vector<unsigned char> &Res,
                               std::string &Dst, unsigned int Padding = 0) {
  Dst = "";
  char Buf[3] = {0};
  for (auto It = Res.crbegin(); It != Res.crend(); It++) {
    snprintf(Buf, 3, "%02x", *It);
    Dst += Buf;
  }
  if (Dst.length() < Padding) {
    Dst = std::string(Padding - Dst.length(), '0').append(Dst);
  }
  return true;
}

inline bool convertStringToHex(std::string &Res,
                               std::vector<unsigned char> &Dst,
                               unsigned int Padding = 2) {
  if (Padding & 0x01U) {
    Padding++;
  }
  Dst.clear();
  if (Res.length() == 0) {
    return true;
  }
  if (Res.length() < Padding) {
    Res = std::string(Padding - Res.length(), '0').append(Res);
  }
  if (Res.length() & 0x01U) {
    Res = '0' + Res;
  }
  for (auto It = Res.crbegin(); It != Res.crend(); It += 2) {
    char CL = *It;
    char CH = *(It + 1);
    Dst.push_back(convertCharToHex(CL) + (convertCharToHex(CH) << 4));
  }
  return true;
}

} // namespace Support
} // namespace SSVM
