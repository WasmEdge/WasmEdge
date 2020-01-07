// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/getCallValue.h"
#include "support/hexstr.h"

namespace SSVM {
namespace Executor {

ErrCode EEIGetCallValue::body(VM::EnvironmentManager &EnvMgr,
                              Instance::MemoryInstance &MemInst,
                              uint32_t ResultOffset) {
  std::vector<unsigned char> Data;
  Support::convertHexStrToValVec(Env.getCallValue(), Data, 32);
  return MemInst.setBytes(Data, ResultOffset, 0, 16);
}

} // namespace Executor
} // namespace SSVM
