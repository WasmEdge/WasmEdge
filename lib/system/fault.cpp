// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "system/fault.h"

#include "common/config.h"
#include "common/defines.h"
#include "common/spdlog.h"
#include "system/stacktrace.h"

#include <atomic>
#include <csetjmp>
#include <csignal>
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
void signalHandler(int Signal, siginfo_t *Siginfo, void *) {
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
  struct sigaction Action{};
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

winapi::LONG_ WASMEDGE_WINAPI_WINAPI_CC
vectoredExceptionHandler(winapi::PEXCEPTION_POINTERS_ ExceptionInfo) {
  const winapi::DWORD_ Code = ExceptionInfo->ExceptionRecord->ExceptionCode;
  switch (Code) {
  case winapi::EXCEPTION_INT_DIVIDE_BY_ZERO_:
    Fault::emitFault(ErrCode::Value::DivideByZero);
  case winapi::EXCEPTION_INT_OVERFLOW_:
    Fault::emitFault(ErrCode::Value::IntegerOverflow);
  case winapi::EXCEPTION_ACCESS_VIOLATION_:
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
  Prev = std::exchange(localHandler, this);
  increaseHandler();
}

Fault::~Fault() noexcept {
  decreaseHandler();
  localHandler = std::exchange(Prev, nullptr);
}

[[noreturn]] void Fault::emitFault(ErrCode Error) {
  assuming(localHandler != nullptr);
  // Signal-safe shadow-root truncation (GC proposal, spec R7-M2). Before the
  // longjmp abandons the compiled frames, reset the thread's shadow-root head
  // to its value at the compiled-call boundary, so a concurrent collector never
  // walks a ShadowFrame in the stack we are about to unwind. A lock-free atomic
  // store is async-signal-safe; no lock is taken and nothing is allocated here.
  static_assert(std::atomic<void *>::is_always_lock_free,
                "shadow-head truncation must be a lock-free (async-signal-safe) "
                "atomic store");
  if (localHandler->ShadowHeadCell != nullptr) {
    localHandler->ShadowHeadCell->store(localHandler->ShadowHeadBoundary,
                                        std::memory_order_release);
  }
  auto Buffer = stackTrace(localHandler->StackTraceBuffer);
  localHandler->StackTraceSize = Buffer.size();
  longjmp(localHandler->Buffer, static_cast<int>(Error.operator uint32_t()));
}

} // namespace WasmEdge
