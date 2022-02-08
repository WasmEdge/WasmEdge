#pragma once

#include "common/enum_types.h"
#include "common/span.h"
#include "common/types.h"
#include "helper.h"
#include "host/wasi_crypto/ctx.h"
#include "runtime/instance/memory.h"
#include "wasi_crypto/api.hpp"
#include "gtest/gtest.h"

#include <cstdint>
#include <queue>
#include <type_traits>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

std::vector<uint8_t> operator"" _u8(const char *Str, std::size_t Len) noexcept;

std::vector<uint8_t> operator"" _u8v(const char *Str, std::size_t Len) noexcept;

/// designed for test
class WasiCryptoTest : public ::testing::Test {
protected:
  void writeDummyMemoryContent() noexcept;

  void writeString(std::string_view String, uint32_t Ptr);

  void writeSpan(Span<const uint8_t> Content, uint32_t Ptr);

  void writeOptKey(std::optional<uint32_t> OptKey, uint32_t Ptr);

  void writeOptOptions(std::optional<__wasi_options_t> OptOptions,
                       uint32_t Ptr);

  template <typename T>
  Expect<__wasi_crypto_errno_e_t>
  testRun(const std::vector<WasmEdge::ValVariant> &Args) {
    static T Function{Ctx};
    if (auto Res = Function.run(&MemInst, Args, Errno); !Res) {
      return Unexpect(Res);
    }

    return static_cast<__wasi_crypto_errno_e_t>(Errno[0].get<uint32_t>());
  }

  uint32_t InvaildHandle = 9999;
  WasmEdge::Runtime::Instance::MemoryInstance MemInst{
      WasmEdge::AST::MemoryType(1)};
  std::array<WasmEdge::ValVariant, 1> Errno;

private:
  WasiCryptoContext Ctx;
};
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