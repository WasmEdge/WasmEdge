// SPDX-License-Identifier: Apache-2.0
#include "compiler/hostfunc/wasi/environ_SizesGet.h"
#include <cstring>
#include <unistd.h>

namespace SSVM {
namespace Compiler {

uint32_t WasiEnvironSizesGet::run(uint32_t EnvCntPtr, uint32_t EnvBufSizePtr) {
  /// Calculate EnvCnt and EnvBufSize.
  uint32_t EnvCnt = 0;
  uint32_t EnvBufSize = 0;
  for (char **Env = environ; *Env != nullptr; ++Env) {
    ++EnvCnt;
    EnvBufSize += std::strlen(*Env) + 1;
  }

  /// Store EnvCnt and EnvBufSize.
  Lib.getMemory<uint32_t>(EnvCntPtr) = EnvCnt;
  Lib.getMemory<uint32_t>(EnvBufSizePtr) = EnvBufSize;

  /// Return: errno(u32)
  return 0;
}

} // namespace Compiler
} // namespace SSVM
