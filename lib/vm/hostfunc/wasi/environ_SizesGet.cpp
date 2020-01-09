// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/wasi/environ_SizesGet.h"
#include <string_view>

extern char **environ;

namespace SSVM {
namespace Executor {

ErrCode WasiEnvironSizesGet::body(VM::EnvironmentManager &EnvMgr,
                                  Instance::MemoryInstance &MemInst,
                                  uint32_t &ErrNo, uint32_t EnvCntPtr,
                                  uint32_t EnvBufSizePtr) {
  /// Calculate EnvCnt and EnvBufSize.
  uint32_t EnvCnt;
  uint32_t EnvBufSize = 0;

  for (EnvCnt = 0; environ[EnvCnt] != nullptr; ++EnvCnt) {
    std::string_view EnvString(environ[EnvCnt]);
    EnvBufSize += EnvString.size() + 1;
  }

  /// Store EnvCnt and EnvBufSize.
  if (ErrCode Status = MemInst.storeValue(EnvCnt, EnvCntPtr, 4);
      Status != ErrCode::Success) {
    return Status;
  }
  if (ErrCode Status = MemInst.storeValue(EnvBufSize, EnvBufSizePtr, 4);
      Status != ErrCode::Success) {
    return Status;
  }

  ErrNo = 0U;
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
