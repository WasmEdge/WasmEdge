// SPDX-License-Identifier: Apache-2.0
#include "compiler/hostfunc/wasi/fd_Close.h"
#include <algorithm>
#include <unistd.h>

namespace SSVM {
namespace Compiler {

uint32_t WasiFdClose::run(uint32_t Fd) {
  /// Close Fd.
  if (close(Fd) != 0) {
    return convertErrno(errno);
  }
  return 0;
}

} // namespace Compiler
} // namespace SSVM
