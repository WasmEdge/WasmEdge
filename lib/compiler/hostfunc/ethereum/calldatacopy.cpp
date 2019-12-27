// SPDX-License-Identifier: Apache-2.0
#include "compiler/hostfunc/ethereum/calldatacopy.h"

namespace SSVM {
namespace Compiler {

void EEICallDataCopy::run(uint32_t ResultOffset, uint32_t DataOffset,
                          uint32_t Length) {
  if (Length > 0) {
    auto Result = Lib.getMemory<uint8_t>(ResultOffset, Length);
    std::copy(Env.getCallData().cbegin() + DataOffset,
              Env.getCallData().cbegin() + DataOffset + Length, Result.begin());
  }
}

} // namespace Compiler
} // namespace SSVM
