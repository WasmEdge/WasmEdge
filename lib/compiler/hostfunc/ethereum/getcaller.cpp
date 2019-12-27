// SPDX-License-Identifier: Apache-2.0
#include "compiler/hostfunc/ethereum/getcaller.h"
#include "support/hexstr.h"

namespace SSVM {
namespace Compiler {

void EEIGetCaller::run(uint32_t ResultOffset) {
  auto Result = Lib.getMemory<uint8_t>(ResultOffset, 20);
  const auto &Caller = Env.getCaller();
  std::copy(Caller.cbegin(), Caller.cend(), Result.begin());
}

} // namespace Compiler
} // namespace SSVM
