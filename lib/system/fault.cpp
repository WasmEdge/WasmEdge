// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "system/fault.h"

#include "common/config.h"
#include "common/defines.h"
#include "common/spdlog.h"
#include "system/stacktrace.h"

#include <atomic>
#include <csetjmp>
#include <csignal>
#include <cstdlib>
#include <cstdint>
#include <utility>

#if WASMEDGE_OS_WINDOWS
#include "system/winapi.h"
#endif

namespace WasmEdge {

namespace {

std::atomic_uint handlerCount = 0;
thread_local Fault *localHandler = nullptr;

#if defined(SA_SIGINFO)
static constexpr std::size_t AltStackSize = 65536;
thread_local char AltStackMem[AltStackSize];
thread_local bool AltStackInstalled = false;

void ensureAltStack() noexcept {
  if (!AltStackInstalled) {
    stack_t SS;
    SS.ss_sp = AltStackMem;
    SS.ss_size = AltStackSize;
    SS.ss_flags = 0;
    sigaltstack(&SS, nullptr);
    AltStackInstalled = true;
  }
}
void signalHandler(int Signal, siginfo_t *Siginfo, void *) {
  if (localHandler == nullptr) {
    std::signal(Signal, SIG_DFL);
    raise(Signal);
    std::_Exit(128 + Signal);
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
    assuming(Siginfo->si_code == FPE_INTDIV);
    Fault::emitFault(ErrCode::Value::DivideByZero);
  default:
    assumingUnreachable();
  }
}

void enableHandler() noexcept {
  struct sigaction Action {};
  Action.sa_sigaction = &signalHandler;
  Action.sa_flags = SA_SIGINFO | SA_ONSTACK;
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
  case winapi::EXCEPTION_STACK_OVERFLOW_:
    Fault::emitFault(ErrCode::Value::MemoryOutOfBounds);
  }
  return winapi::EXCEPTION_CONTINUE_EXECUTION_;
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
#if defined(SA_SIGINFO)
  ensureAltStack();
#endif
  Prev = std::exchange(localHandler, this);
  increaseHandler();
}

Fault::~Fault() noexcept {
  decreaseHandler();
  localHandler = std::exchange(Prev, nullptr);
}

[[noreturn]] void Fault::emitFault(ErrCode Error) {
  if (localHandler == nullptr) {
    std::signal(SIGABRT, SIG_DFL);
    std::abort();
  }
  auto Buffer = stackTrace(localHandler->StackTraceBuffer);
  localHandler->StackTraceSize = Buffer.size();
  longjmp(localHandler->Buffer, static_cast<int>(Error.operator uint32_t()));
}

} // namespace WasmEdge
