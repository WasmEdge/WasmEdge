// SPDX-License-Identifier: Apache-2.0
#include "compiler/hostfunc/wasi/environ_Get.h"

#include <unistd.h>

namespace SSVM {
namespace Compiler {

uint32_t WasiEnvironGet::run(uint32_t EnvPtr, uint32_t EnvBufPtr) {
  std::vector<uint32_t> VEnv;
  std::vector<uint8_t> VEnvBuf;

  /// Store **Env.
  for (char **Env = environ; *Env != nullptr; ++Env) {
    /// Calculate Env[i] offset.
    VEnv.push_back(EnvBufPtr + VEnvBuf.size());

    /// Concate EnvString.
    std::copy(*Env, *Env + strlen(*Env), std::back_inserter(VEnvBuf));
    VEnvBuf.push_back('\0');
  }

  /// Store EnvBuf.
  auto Env = Lib.getMemory<uint32_t>(EnvPtr, VEnv.size());
  std::copy(VEnv.cbegin(), VEnv.cend(), Env.begin());
  auto EnvBuf = Lib.getMemory<uint8_t>(EnvBufPtr, VEnvBuf.size());
  std::copy(VEnvBuf.cbegin(), VEnvBuf.cend(), EnvBuf.begin());

  /// Return: errno(u32)
  return 0;
}

} // namespace Compiler
} // namespace SSVM
