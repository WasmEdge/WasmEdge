// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/getCaller.h"
#include "support/hexstr.h"

namespace SSVM {
namespace Executor {

ErrCode EEIGetCaller::body(VM::EnvironmentManager &EnvMgr,
                           Instance::MemoryInstance &MemInst,
                           uint32_t ResultOffset) {
  std::vector<unsigned char> Data;
  Support::convertHexStrToBytes(Env.getCaller(), Data, 40);
  return MemInst.setBytes(Data, ResultOffset, 0, 20);
}

} // namespace Executor
} // namespace SSVM
