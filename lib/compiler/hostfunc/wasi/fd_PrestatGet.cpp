// SPDX-License-Identifier: Apache-2.0
#include "compiler/hostfunc/wasi/fd_PrestatGet.h"
#include <sys/stat.h>

namespace SSVM {
namespace Compiler {

uint32_t WasiFdPrestatGet::run(uint32_t Fd, uint32_t PreStatPtr) {
  for (const auto &Entry : Env.getPreStats()) {
    if (Entry.Fd != Fd) {
      continue;
    }
    struct stat SysFStat;
    if (fstat(Fd, &SysFStat) != 0) {
      return convertErrno(errno);
    }
    /// byte[0] : pr_type(uint8_t)
    Lib.getMemory<uint8_t>(PreStatPtr) =
        ((SysFStat.st_mode & S_IFMT) == S_IFDIR) ? __WASI_PREOPENTYPE_DIR : 1;
    /// byte[4:8] : u.dir.pr_name_len(size_t)
    Lib.getMemory<uint32_t>(PreStatPtr + 4) = Entry.Path.size();
    return 0;
  }
  return __WASI_EBADF;
}

} // namespace Compiler
} // namespace SSVM
