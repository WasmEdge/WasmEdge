// SPDX-License-Identifier: Apache-2.0
#include "compiler/hostfunc/wasi/args_Get.h"

namespace SSVM {
namespace Compiler {

uint32_t WasiArgsGet::run(uint32_t ArgvPtr, uint32_t ArgvBufPtr) {
  /// Store **Argv.
  std::vector<std::string> &CmdArgs = Env.getCmdArgs();
  std::vector<uint32_t> VArgv;
  std::vector<unsigned char> VArgvBuf;
  for (auto It = CmdArgs.cbegin(); It != CmdArgs.cend(); It++) {
    /// Concate Argv.
    VArgv.push_back(ArgvBufPtr + VArgvBuf.size());
    std::copy(It->cbegin(), It->cend(), std::back_inserter(VArgvBuf));
    VArgvBuf.push_back('\0');
  }
  VArgv.push_back(0);

  /// Store ArgvBuf.
  auto Argv = Lib.getMemory<uint32_t>(ArgvPtr, VArgv.size());
  std::copy(VArgv.cbegin(), VArgv.cend(), Argv.begin());
  auto ArgvBuf = Lib.getMemory<uint8_t>(ArgvBufPtr, VArgvBuf.size());
  std::copy(VArgvBuf.cbegin(), VArgvBuf.cend(), ArgvBuf.begin());

  return 0;
}

} // namespace Compiler
} // namespace SSVM
