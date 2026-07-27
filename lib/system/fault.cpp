// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "system/fault.h"

#include "common/config.h"
#include "common/defines.h"
#include "common/spdlog.h"
#include "system/stacktrace.h"

#include <array>
#include <csetjmp>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <utility>

#if defined(SA_SIGINFO)
#include <pthread.h>
#endif

#if WASMEDGE_OS_WINDOWS
#include "system/winapi.h"
#endif

namespace WasmEdge {

namespace {

std::mutex HandlerMutex;
uint32_t HandlerCount = 0;
thread_local Fault *localHandler = nullptr;

#if defined(SA_SIGINFO)

constexpr std::array FaultSignals{SIGFPE, SIGBUS, SIGSEGV};
std::array<struct sigaction, FaultSignals.size()> PreviousActions{};

size_t signalIndex(int Signal) noexcept {
  for (size_t I = 0; I < FaultSignals.size(); ++I) {
    if (FaultSignals[I] == Signal) {
      return I;
    }
  }
  assumingUnreachable();
}

[[noreturn]] void raiseDefaultSignal(int Signal,
                                     const struct sigaction &Action) noexcept {
  sigaction(Signal, &Action, nullptr);
  sigset_t Set;
  sigemptyset(&Set);
  sigaddset(&Set, Signal);
  pthread_sigmask(SIG_UNBLOCK, &Set, nullptr);
  raise(Signal);
  std::_Exit(128 + Signal);
}

void forwardSignal(int Signal, siginfo_t *Siginfo, void *Context) noexcept {
  const auto &Action = PreviousActions[signalIndex(Signal)];
  if (Action.sa_handler == SIG_IGN) {
    return;
  }
  if (Action.sa_handler == SIG_DFL) {
    raiseDefaultSignal(Signal, Action);
  }

  sigset_t Set = Action.sa_mask;
  if ((Action.sa_flags & SA_NODEFER) == 0) {
    sigaddset(&Set, Signal);
  }
  pthread_sigmask(SIG_BLOCK, &Set, nullptr);

  if ((Action.sa_flags & SA_SIGINFO) != 0) {
    Action.sa_sigaction(Signal, Siginfo, Context);
  } else {
    Action.sa_handler(Signal);
  }
}

void signalHandler(int Signal, siginfo_t *Siginfo, void *Context) {
  if (localHandler == nullptr) {
    forwardSignal(Signal, Siginfo, Context);
    return;
  }

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
    Fault::emitFault(ErrCode::Value::MemoryOutOfBounds);
  case SIGFPE:
    if (Siginfo != nullptr && Siginfo->si_code == FPE_INTDIV) {
      Fault::emitFault(ErrCode::Value::DivideByZero);
    }
    forwardSignal(Signal, Siginfo, Context);
    return;
  default:
    assumingUnreachable();
  }
}

void enableHandler() noexcept {
  struct sigaction Action{};
  sigemptyset(&Action.sa_mask);
  Action.sa_sigaction = &signalHandler;
  Action.sa_flags = SA_SIGINFO | SA_ONSTACK;
  for (size_t I = 0; I < FaultSignals.size(); ++I) {
    sigaction(FaultSignals[I], &Action, &PreviousActions[I]);
  }
}

void disableHandler() noexcept {
  for (size_t I = 0; I < FaultSignals.size(); ++I) {
    sigaction(FaultSignals[I], &PreviousActions[I], nullptr);
  }
}

#elif WASMEDGE_OS_WINDOWS

winapi::LONG_ WASMEDGE_WINAPI_WINAPI_CC
vectoredExceptionHandler(winapi::PEXCEPTION_POINTERS_ ExceptionInfo) {
  if (localHandler == nullptr) {
    return winapi::EXCEPTION_CONTINUE_SEARCH_;
  }
  const winapi::DWORD_ Code = ExceptionInfo->ExceptionRecord->ExceptionCode;
  switch (Code) {
  case winapi::EXCEPTION_INT_DIVIDE_BY_ZERO_:
    Fault::emitFault(ErrCode::Value::DivideByZero);
  case winapi::EXCEPTION_INT_OVERFLOW_:
    Fault::emitFault(ErrCode::Value::IntegerOverflow);
  case winapi::EXCEPTION_ACCESS_VIOLATION_:
    Fault::emitFault(ErrCode::Value::MemoryOutOfBounds);
  }
  return winapi::EXCEPTION_CONTINUE_SEARCH_;
}

void *HandlerHandle = nullptr;

void enableHandler() noexcept {
  HandlerHandle =
      winapi::AddVectoredExceptionHandler(1, &vectoredExceptionHandler);
}

void disableHandler() noexcept {
  winapi::RemoveVectoredExceptionHandler(HandlerHandle);
}

#endif

void increaseHandler() noexcept {
  std::lock_guard Lock(HandlerMutex);
  if (HandlerCount++ == 0) {
    enableHandler();
  }
}

void decreaseHandler() noexcept {
  std::lock_guard Lock(HandlerMutex);
  if (--HandlerCount == 0) {
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

[[noreturn]] void Fault::emitFault(ErrCode Error) {
  assuming(localHandler != nullptr);
  auto Buffer = stackTrace(localHandler->StackTraceBuffer);
  localHandler->StackTraceSize = Buffer.size();
  longjmp(localHandler->Buffer, static_cast<int>(Error.operator uint32_t()));
}

} // namespace WasmEdge
