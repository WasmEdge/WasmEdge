// SPDX-License-Identifier: Apache-2.0
#include "compiler/hostfunc/wasi/args_SizesGet.h"

namespace SSVM {
namespace Compiler {

uint32_t WasiArgsSizesGet::run(uint32_t ArgcPtr, uint32_t ArgvBufSizePtr) {
  /// Store Argc.
  std::vector<std::string> &CmdArgs = Env.getCmdArgs();
  Lib.getMemory<uint32_t>(ArgcPtr) = CmdArgs.size();

  /// Store ArgvBufSize.
  uint32_t CmdArgsSize = 0;
  for (auto It = CmdArgs.cbegin(); It != CmdArgs.cend(); It++) {
    CmdArgsSize += It->length() + 1;
  }
  Lib.getMemory<uint32_t>(ArgvBufSizePtr) = CmdArgsSize;

  return 0;
}

} // namespace Compiler
} // namespace SSVM
