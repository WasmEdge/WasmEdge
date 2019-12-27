// SPDX-License-Identifier: Apache-2.0
#include "compiler/hostfunc/ethereum/returndatacopy.h"

namespace SSVM {
namespace Compiler {

void EEIReturnDataCopy::run(uint32_t ResultOffset, uint32_t DataOffset,
                            uint32_t Length) {

  if (Length > 0) {
    auto Result = Lib.getMemory<uint8_t>(ResultOffset, 20);
    std::copy(Env.getReturnData().cbegin() + DataOffset,
              Env.getReturnData().cbegin() + DataOffset + Length,
              Result.begin());
  }
}

} // namespace Compiler
} // namespace SSVM
