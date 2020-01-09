// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/wasi/args_Get.h"

namespace SSVM {
namespace Executor {

ErrCode WasiArgsGet::body(VM::EnvironmentManager &EnvMgr,
                          Instance::MemoryInstance &MemInst, uint32_t &ErrNo,
                          uint32_t ArgvPtr, uint32_t ArgvBufPtr) {
  /// Store **Argv.
  std::vector<unsigned char> ArgvBuf;
  uint32_t ArgvBufOffset = ArgvBufPtr;
  for (const auto &Arg : Env.getCmdArgs()) {
    /// Concate Argv.
    std::copy(Arg.cbegin(), Arg.cend(), std::back_inserter(ArgvBuf));
    ArgvBuf.push_back('\0');

    /// Calcuate Argv[i] offset and store.
    if (ErrCode Status = MemInst.storeValue(ArgvBufOffset, ArgvPtr, 4);
        Status != ErrCode::Success) {
      return Status;
    }

    /// Shift one element.
    ArgvBufOffset += Arg.size() + 1;
    ArgvPtr += 4;
  }

  /// Store nullptr
  if (ErrCode Status = MemInst.storeValue(uint32_t(0), ArgvPtr, 4);
      Status != ErrCode::Success) {
    return Status;
  }

  /// Store ArgvBuf.
  if (ErrCode Status = MemInst.setBytes(ArgvBuf, ArgvBufPtr, 0, ArgvBuf.size());
      Status != ErrCode::Success) {
    return Status;
  }

  ErrNo = 0U;
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
