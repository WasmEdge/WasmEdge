// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/wasi/environ_Get.h"
#include <string_view>

extern char **environ;

namespace SSVM {
namespace Executor {

ErrCode WasiEnvironGet::body(VM::EnvironmentManager &EnvMgr,
                             Instance::MemoryInstance &MemInst, uint32_t &ErrNo,
                             uint32_t EnvPtr, uint32_t EnvBufPtr) {
  /// Store **Env.
  uint32_t EnvBufOffset = EnvBufPtr;
  std::vector<unsigned char> EnvBuf;
  for (uint32_t EnvCnt = 0; environ[EnvCnt] != nullptr; ++EnvCnt) {
    std::string_view EnvString(environ[EnvCnt]);

    /// Concate EnvString.
    std::copy(EnvString.cbegin(), EnvString.cend(), std::back_inserter(EnvBuf));
    EnvBuf.push_back('\0');

    /// Calculate Env[i] offset and store.
    if (ErrCode Status = MemInst.storeValue(EnvBufOffset, EnvPtr, 4);
        Status != ErrCode::Success) {
      return Status;
    }

    /// Shift one element.
    EnvBufOffset += EnvString.size() + 1;
    EnvPtr += 4;
  }

  /// Store nullptr
  if (ErrCode Status = MemInst.storeValue(uint32_t(0), EnvPtr, 4);
      Status != ErrCode::Success) {
    return Status;
  }

  /// Store EnvBuf.
  if (ErrCode Status = MemInst.setBytes(EnvBuf, EnvBufPtr, 0, EnvBuf.size());
      Status != ErrCode::Success) {
    return Status;
  }

  ErrNo = 0U;
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
