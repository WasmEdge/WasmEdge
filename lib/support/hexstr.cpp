#include "support/hexstr.h"
#include <cinttypes>

namespace SSVM {
namespace Support {

uint8_t convertCharToHex(const char C) {
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

void convertBytesToHexStr(Span<const uint8_t> Src, std::string &Dst,
                          const uint32_t Padding, const bool IsLittleEndian) {
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

void convertHexStrToValVec(std::string_view Src, std::vector<uint8_t> &Dst,
                           const uint32_t Padding) {
  convertHexStrToBytes(Src, Dst, Padding);
}

std::string convertUIntToHexStr(const uint64_t Num, uint32_t MinLen) {
  char Str[32];
  const int FieldWidth = std::min(MinLen, UINT32_C(16));
  std::sprintf(Str, "0x%0*" PRIu64 "x", FieldWidth, Num);
  return Str;
}

} // namespace Support
} // namespace SSVM
