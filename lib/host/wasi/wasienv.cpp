// SPDX-License-Identifier: Apache-2.0
#include "host/wasi/wasienv.h"

namespace SSVM {
namespace Host {

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

} // namespace Host
} // namespace SSVM