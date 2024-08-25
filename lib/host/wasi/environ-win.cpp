// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/defines.h"
#if WASMEDGE_OS_WINDOWS

#include "common/errcode.h"
#include "host/wasi/environ.h"
#include "win.h"
#include <csignal>

using namespace WasmEdge::winapi;

namespace WasmEdge {
namespace Host {
namespace WASI {

WasiExpect<void> Environ::procRaise(__wasi_signal_t Signal) const noexcept {
  int SysSignal;
  switch (Signal) {
  case __WASI_SIGNAL_NONE:
    SysSignal = 0;
    break;
  case __WASI_SIGNAL_INT:
    SysSignal = SIGINT;
    break;
  case __WASI_SIGNAL_ILL:
    SysSignal = SIGILL;
    break;
  case __WASI_SIGNAL_ABRT:
    SysSignal = SIGABRT;
    break;
  case __WASI_SIGNAL_FPE:
    SysSignal = SIGFPE;
    break;
  case __WASI_SIGNAL_SEGV:
    SysSignal = SIGSEGV;
    break;
  case __WASI_SIGNAL_TERM:
    SysSignal = SIGTERM;
    break;
  case __WASI_SIGNAL_HUP:
  case __WASI_SIGNAL_QUIT:
  case __WASI_SIGNAL_TRAP:
  case __WASI_SIGNAL_BUS:
  case __WASI_SIGNAL_KILL:
  case __WASI_SIGNAL_USR1:
  case __WASI_SIGNAL_USR2:
  case __WASI_SIGNAL_PIPE:
  case __WASI_SIGNAL_ALRM:
  case __WASI_SIGNAL_CHLD:
  case __WASI_SIGNAL_CONT:
  case __WASI_SIGNAL_STOP:
  case __WASI_SIGNAL_TSTP:
  case __WASI_SIGNAL_TTIN:
  case __WASI_SIGNAL_TTOU:
  case __WASI_SIGNAL_URG:
  case __WASI_SIGNAL_XCPU:
  case __WASI_SIGNAL_XFSZ:
  case __WASI_SIGNAL_VTALRM:
  case __WASI_SIGNAL_PROF:
  case __WASI_SIGNAL_WINCH:
  case __WASI_SIGNAL_POLL:
  case __WASI_SIGNAL_PWR:
  case __WASI_SIGNAL_SYS:
  default:
    return WasiUnexpect(__WASI_ERRNO_NOSYS);
  }
  if (auto Res = std::raise(SysSignal); Res != 0) {
    return WasiUnexpect(fromErrNo(errno));
  }
  return {};
}

WasiExpect<void> Environ::schedYield() const noexcept {
  SwitchToThread();
  return {};
}

} // namespace WASI
} // namespace Host
} // namespace WasmEdge

#endif
