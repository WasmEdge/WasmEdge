// SPDX-License-Identifier: Apache-2.0
#include "compiler/hostfunc/wasi/fd_PrestatDirName.h"
#include <cstring>

namespace SSVM {
namespace Compiler {

uint32_t WasiFdPrestatDirName::run(uint32_t Fd, uint32_t PathBufPtr,
                                   uint32_t PathLen) {
  for (auto &Entry : Env.getPreStats()) {
    if (Entry.Fd != Fd) {
      continue;
    }
    if (Entry.Path.size() > PathLen) {
      return __WASI_EINVAL;
    }

    std::copy(Entry.Path.cbegin(), Entry.Path.cend(),
              Lib.getMemory<char>(PathBufPtr, Entry.Path.size()).begin());
    return 0;
  }
  return __WASI_EBADF;
}

} // namespace Compiler
} // namespace SSVM
