// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/wasi/args_SizesGet.h"

namespace SSVM {
namespace Executor {

ErrCode WasiArgsSizesGet::body(VM::EnvironmentManager &EnvMgr,
                               Instance::MemoryInstance &MemInst,
                               uint32_t &ErrNo, uint32_t ArgcPtr,
                               uint32_t ArgvBufSizePtr) {
  /// Store Argc.
  std::vector<std::string> &CmdArgs = Env.getCmdArgs();
  if (ErrCode Status = MemInst.storeValue(uint32_t(CmdArgs.size()), ArgcPtr, 4);
      Status != ErrCode::Success) {
    return Status;
  }

  /// Store ArgvBufSize.
  uint32_t CmdArgsSize = 0;
  for (const auto &Arg : CmdArgs) {
    CmdArgsSize += Arg.size() + 1;
  }
  if (ErrCode Status = MemInst.storeValue(CmdArgsSize, ArgvBufSizePtr, 4);
      Status != ErrCode::Success) {
    return Status;
  }

  ErrNo = 0U;
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
