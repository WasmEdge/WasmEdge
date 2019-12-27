// SPDX-License-Identifier: Apache-2.0
#include "compiler/hostfunc/wasi/proc_Exit.h"

namespace SSVM {
namespace Compiler {

void WasiProcExit::run(uint32_t ExitCode) {
  Env.setExitCode(ExitCode);
  Lib.terminate();
}

} // namespace Compiler
} // namespace SSVM
