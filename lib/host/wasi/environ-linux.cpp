// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/defines.h"
#if WASMEDGE_OS_LINUX

#include "common/errcode.h"
#include "host/wasi/environ.h"
#include "linux.h"

namespace WasmEdge {
namespace Host {
namespace WASI {

WasiExpect<void> Environ::procRaise(__wasi_signal_t Signal) const noexcept {
  int SysSignal;
  switch (Signal) {
  case __WASI_SIGNAL_NONE:
    SysSignal = 0;
    break;
  case __WASI_SIGNAL_HUP:
    SysSignal = SIGHUP;
    break;
  case __WASI_SIGNAL_INT:
    SysSignal = SIGINT;
    break;
  case __WASI_SIGNAL_QUIT:
    SysSignal = SIGQUIT;
    break;
  case __WASI_SIGNAL_ILL:
    SysSignal = SIGILL;
    break;
  case __WASI_SIGNAL_TRAP:
    SysSignal = SIGTRAP;
    break;
  case __WASI_SIGNAL_ABRT:
    SysSignal = SIGABRT;
    break;
  case __WASI_SIGNAL_BUS:
    SysSignal = SIGBUS;
    break;
  case __WASI_SIGNAL_FPE:
    SysSignal = SIGFPE;
    break;
  case __WASI_SIGNAL_KILL:
    SysSignal = SIGKILL;
    break;
  case __WASI_SIGNAL_USR1:
    SysSignal = SIGUSR1;
    break;
  case __WASI_SIGNAL_SEGV:
    SysSignal = SIGSEGV;
    break;
  case __WASI_SIGNAL_USR2:
    SysSignal = SIGUSR2;
    break;
  case __WASI_SIGNAL_PIPE:
    SysSignal = SIGPIPE;
    break;
  case __WASI_SIGNAL_ALRM:
    SysSignal = SIGALRM;
    break;
  case __WASI_SIGNAL_TERM:
    SysSignal = SIGTERM;
    break;
  case __WASI_SIGNAL_CHLD:
    SysSignal = SIGCHLD;
    break;
  case __WASI_SIGNAL_CONT:
    SysSignal = SIGCONT;
    break;
  case __WASI_SIGNAL_STOP:
    SysSignal = SIGSTOP;
    break;
  case __WASI_SIGNAL_TSTP:
    SysSignal = SIGTSTP;
    break;
  case __WASI_SIGNAL_TTIN:
    SysSignal = SIGTTIN;
    break;
  case __WASI_SIGNAL_TTOU:
    SysSignal = SIGTTOU;
    break;
  case __WASI_SIGNAL_URG:
    SysSignal = SIGURG;
    break;
  case __WASI_SIGNAL_XCPU:
    SysSignal = SIGXCPU;
    break;
  case __WASI_SIGNAL_XFSZ:
    SysSignal = SIGXFSZ;
    break;
  case __WASI_SIGNAL_VTALRM:
    SysSignal = SIGVTALRM;
    break;
  case __WASI_SIGNAL_PROF:
    SysSignal = SIGPROF;
    break;
  case __WASI_SIGNAL_WINCH:
    SysSignal = SIGWINCH;
    break;
  case __WASI_SIGNAL_POLL:
    SysSignal = SIGPOLL;
    break;
  case __WASI_SIGNAL_PWR:
    SysSignal = SIGPWR;
    break;
  case __WASI_SIGNAL_SYS:
    SysSignal = SIGSYS;
    break;
  default:
    return WasiUnexpect(__WASI_ERRNO_NOTSUP);
  }
  if (auto Res = std::raise(SysSignal); Res != 0) {
    return WasiUnexpect(fromErrNo(errno));
  }
  return {};
}

WasiExpect<void> Environ::schedYield() const noexcept {
  ::sched_yield();
  return {};
}

} // namespace WASI
} // namespace Host
} // namespace WasmEdge

#endif
