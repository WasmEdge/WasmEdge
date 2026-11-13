// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "host/wasi/component/env.h"
#include "common/defines.h"

#if WASMEDGE_OS_WINDOWS
#include "system/winapi.h"
#else
#include <unistd.h>
#endif

namespace WasmEdge {
namespace Host {
namespace WasiComponent {

bool Env::isTerminal(uint32_t Fd) const noexcept {
  auto Node = stdioNode(Fd);
  if (!Node) {
    return false;
  }
  auto Handle = Node->getNativeHandler();
  if (!Handle) {
    return false;
  }
#if WASMEDGE_OS_WINDOWS
  winapi::DWORD_ Mode = 0;
  return winapi::GetConsoleMode(
             reinterpret_cast<winapi::HANDLE_>(static_cast<uintptr_t>(*Handle)),
             &Mode) != 0;
#else
  return ::isatty(static_cast<int>(*Handle)) == 1;
#endif
}

} // namespace WasiComponent
} // namespace Host
} // namespace WasmEdge
