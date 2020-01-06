// SPDX-License-Identifier: Apache-2.0
#include "vm/environment.h"
#include <fcntl.h>
#include <unistd.h>
#include <wasi/core.h>

namespace SSVM {
namespace VM {

WasiEnvironment::WasiEnvironment() {
  /// Open dir for WASI environment.
  PreStats.emplace_back(open(".", O_RDONLY | O_DIRECTORY),
                        __WASI_PREOPENTYPE_DIR,
                        std::vector<unsigned char>({'.'}));
}

WasiEnvironment::~WasiEnvironment() noexcept {
  for (const auto &Entry : PreStats) {
    close(Entry.Fd);
  }
}

} // namespace VM
} // namespace SSVM
