// SPDX-License-Identifier: Apache-2.0
#include "system/fault.h"
#include "common/defines.h"
#include "common/log.h"
#include "config.h"
#include <atomic>
#include <cassert>
#include <csetjmp>
#include <csignal>

#if WASMEDGE_OS_WINDOWS
#include <windows.h>

#include <errhandlingapi.h>
#include <winnt.h>
#endif

namespace WasmEdge {

namespace {

std::atomic_uint handlerCount = 0;
thread_local Fault *localHandler = nullptr;

#if defined(SA_SIGINFO)
[[noreturn]] void signalHandler(int Signal, siginfo_t *Siginfo [[maybe_unused]],
                                void *) noexcept {
  {
    // Unblock current signal
    sigset_t Set;
    sigemptyset(&Set);
    sigaddset(&Set, Signal);
    pthread_sigmask(SIG_UNBLOCK, &Set, nullptr);
  }
  switch (Signal) {
  case SIGBUS:
  case SIGSEGV:
    Fault::emitFault(ErrCode::MemoryOutOfBounds);
  case SIGFPE:
    assert(Siginfo->si_code == FPE_INTDIV);
    Fault::emitFault(ErrCode::DivideByZero);
  default:
    __builtin_unreachable();
  }
}

void enableHandler() noexcept {
  struct sigaction Action {};
  Action.sa_sigaction = &signalHandler;
  Action.sa_flags = SA_SIGINFO;
  sigaction(SIGFPE, &Action, nullptr);
  sigaction(SIGBUS, &Action, nullptr);
  sigaction(SIGSEGV, &Action, nullptr);
}

void disableHandler() noexcept {
  std::signal(SIGFPE, SIG_DFL);
  std::signal(SIGBUS, SIG_DFL);
  std::signal(SIGSEGV, SIG_DFL);
}

#elif WASMEDGE_OS_WINDOWS

LONG vectoredExceptionHandler(EXCEPTION_POINTERS *ExceptionInfo) {
  const DWORD Code = ExceptionInfo->ExceptionRecord->ExceptionCode;
  switch (Code) {
  case EXCEPTION_INT_DIVIDE_BY_ZERO:
    Fault::emitFault(ErrCode::DivideByZero);
  case EXCEPTION_INT_OVERFLOW:
    Fault::emitFault(ErrCode::IntegerOverflow);
  case EXCEPTION_ACCESS_VIOLATION:
    Fault::emitFault(ErrCode::MemoryOutOfBounds);
  }
  return EXCEPTION_CONTINUE_EXECUTION;
}

void *HandlerHandle = nullptr;

void enableHandler() noexcept {
  HandlerHandle = AddVectoredExceptionHandler(1, &vectoredExceptionHandler);
}

void disableHandler() noexcept {
  RemoveVectoredExceptionHandler(HandlerHandle);
}

#endif

void increaseHandler() noexcept {
  if (handlerCount++ == 0) {
    enableHandler();
  }
}

void decreaseHandler() noexcept {
  if (--handlerCount == 0) {
    disableHandler();
  }
}

} // namespace

Fault::Fault() {
  Prev = std::exchange(localHandler, this);
  increaseHandler();
}

Fault::~Fault() noexcept {
  decreaseHandler();
  localHandler = std::exchange(Prev, nullptr);
}

[[noreturn]] inline void Fault::emitFault(ErrCode Error) {
  assert(localHandler != nullptr);
  longjmp(localHandler->Buffer, uint8_t(Error));
}

FaultBlocker::FaultBlocker() noexcept {
  decreaseHandler();
  Prev = std::exchange(localHandler, nullptr);
}

FaultBlocker::~FaultBlocker() noexcept {
  localHandler = std::exchange(Prev, nullptr);
  increaseHandler();
}

} // namespace WasmEdge
