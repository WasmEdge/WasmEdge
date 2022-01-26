#pragma once
#include "common/span.h"
#include "host/wasi_crypto/ctx.h"
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
inline std::vector<uint8_t> tagToVec(WasiCryptoContext &Ctx,
                                     __wasi_symmetric_tag_t TagHandle) {
  auto SymmetricTagSize = Ctx.symmetricTagLen(TagHandle).value();
  std::vector<uint8_t> Bytes(SymmetricTagSize, 0);
  Ctx.symmetricTagPull(TagHandle, Bytes).value();
  return Bytes;
}

inline Span<uint8_t const> operator"" _u8(const char *Str,
                                          std::size_t Len) noexcept {
  return {reinterpret_cast<uint8_t const *>(Str), Len};
}

inline std::vector<uint8_t> operator"" _u8v(const char *Str,
                                            std::size_t Len) noexcept {
  std::vector<uint8_t> Res(Len / 2);
  for (size_t I = 0; I < Len; I += 2) {
    std::string Tran{Str + I, 2};
    Res[I / 2] = static_cast<uint8_t>(std::strtol(Tran.c_str(), nullptr, 16));
  }
  return Res;
}

// inline std::ostream &operator<<(std::ostream &Os,
//                                 const std::vector<uint8_t> &Vec) {
//   for (size_t Index = 0; Index <= Vec.size(); Index += 15) {
//     std::cout << "              ";
//     auto Diff = Vec.size() - Index;
//     if (Diff >= 15) {
//       for (auto B = Vec.begin() + Index; B < Vec.begin() + Index + 15; ++B) {
//         Os << std::setw(2) << std::setfill('0') << std::hex
//            << static_cast<unsigned int>(*B) << ":";
//       }
//     } else {
//       for (auto B = Vec.begin() + Index; B < Vec.end(); ++B) {
//         Os << std::setw(2) << std::setfill('0') << std::hex
//            << static_cast<unsigned int>(*B) << ":";
//       }
//     }
//     std::cout << "\n";
//   }
//   return Os;
// }
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge