// SPDX-License-Identifier: Apache-2.0
#include "compiler/hostfunc/ethereum/callstatic.h"
#include "keccak/Keccak.h"
#include "support/hexstr.h"
#include <boost/multiprecision/cpp_int.hpp>

namespace SSVM {
namespace Compiler {

uint32_t EEICallStatic::run(uint32_t Gas, uint32_t AddressOffset,
                            uint32_t DataOffset, uint32_t DataLength) {
  auto Address = Lib.getMemory<uint8_t>(AddressOffset, 20);
  auto Data = Lib.getMemory<uint8_t>(DataOffset, DataLength);

  std::array<uint8_t, 20> Addr;
  std::copy(Address.begin(), Address.end(), Addr.begin());

  boost::multiprecision::uint256_t AddressNum = 0;
  for (auto It = Addr.crbegin(); It != Addr.crend(); It++) {
    AddressNum <<= 8;
    AddressNum += *It;
  }

  if (AddressNum == 9) {
    /// Run Keccak
    Keccak K(256);
    for (auto It = Data.begin(); It != Data.end(); It++) {
      K.addData(*It);
    }
    Env.getReturnData() = K.digest();
    return 0U;
  }

  return 1U;
}

} // namespace Compiler
} // namespace SSVM
