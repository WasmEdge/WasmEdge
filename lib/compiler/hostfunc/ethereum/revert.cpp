// SPDX-License-Identifier: Apache-2.0
#include "compiler/hostfunc/ethereum/revert.h"

namespace SSVM {
namespace Compiler {

void EEIRevert::run(uint32_t DataOffset, uint32_t DataLength) {
  auto Data = Lib.getMemory<uint8_t>(DataOffset, DataLength);
  Env.getReturnData().resize(DataLength);
  std::copy(Data.begin(), Data.end(), Env.getReturnData().begin());
}

} // namespace Compiler
} // namespace SSVM
