// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "system/fault.h"

#include "common/config.h"
#include "common/defines.h"
#include "common/log.h"

#include <atomic>
#include <csetjmp>
#include <csignal>
#include <cstdint>
#include <utility>

#if WASMEDGE_OS_WINDOWS

#include <boost/winapi/basic_types.hpp>

#if !defined(BOOST_USE_WINDOWS_H)
extern "C" {

struct _CONTEXT;
struct _EXCEPTION_RECORD;
struct _EXCEPTION_POINTERS;

BOOST_WINAPI_IMPORT boost::winapi::PVOID_ BOOST_WINAPI_WINAPI_CC
AddVectoredExceptionHandler(
    boost::winapi::ULONG_ First,
    boost::winapi::LONG_(BOOST_WINAPI_WINAPI_CC *Handler)(
        struct _EXCEPTION_POINTERS *ExceptionInfo));

BOOST_WINAPI_IMPORT boost::winapi::ULONG_ BOOST_WINAPI_WINAPI_CC
RemoveVectoredExceptionHandler(boost::winapi::PVOID_ Handle);
}
#else
#include <windows.h>

#include <errhandlingapi.h>
#include <winnt.h>
#endif

namespace boost::winapi {

#if defined(BOOST_USE_WINDOWS_H)
BOOST_CONSTEXPR_OR_CONST DWORD_ EXCEPTION_MAXIMUM_PARAMETERS_ =
    EXCEPTION_MAXIMUM_PARAMETERS;
BOOST_CONSTEXPR_OR_CONST DWORD_ EXCEPTION_ACCESS_VIOLATION_ =
    EXCEPTION_ACCESS_VIOLATION;
BOOST_CONSTEXPR_OR_CONST DWORD_ EXCEPTION_INT_DIVIDE_BY_ZERO_ =
    EXCEPTION_INT_DIVIDE_BY_ZERO;
BOOST_CONSTEXPR_OR_CONST DWORD_ EXCEPTION_INT_OVERFLOW_ =
    EXCEPTION_INT_OVERFLOW;
BOOST_CONSTEXPR_OR_CONST LONG_ EXCEPTION_CONTINUE_EXECUTION_ =
    EXCEPTION_CONTINUE_EXECUTION;
#else
BOOST_CONSTEXPR_OR_CONST DWORD_ EXCEPTION_MAXIMUM_PARAMETERS_ = 15;
BOOST_CONSTEXPR_OR_CONST DWORD_ EXCEPTION_ACCESS_VIOLATION_ = 0xC0000005L;
BOOST_CONSTEXPR_OR_CONST DWORD_ EXCEPTION_INT_DIVIDE_BY_ZERO_ = 0xC0000094L;
BOOST_CONSTEXPR_OR_CONST DWORD_ EXCEPTION_INT_OVERFLOW_ = 0xC0000095L;
BOOST_CONSTEXPR_OR_CONST LONG_ EXCEPTION_CONTINUE_EXECUTION_ =
    static_cast<LONG_>(0xffffffff);
#endif

typedef struct BOOST_MAY_ALIAS _CONTEXT CONTEXT_, *PCONTEXT_;

typedef struct BOOST_MAY_ALIAS _EXCEPTION_RECORD {
  DWORD_ ExceptionCode;
  DWORD_ ExceptionFlags;
  struct _EXCEPTION_RECORD *ExceptionRecord;
  PVOID_ ExceptionAddress;
  DWORD_ NumberParameters;
  PULONG_ ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS_];
} EXCEPTION_RECORD_, *PEXCEPTION_RECORD_;

typedef struct BOOST_MAY_ALIAS _EXCEPTION_POINTERS {
  PEXCEPTION_RECORD_ ExceptionRecord;
  PCONTEXT_ ContextRecord;
} EXCEPTION_POINTERS_, *PEXCEPTION_POINTERS_;

BOOST_FORCEINLINE PVOID_ AddVectoredExceptionHandler(
    ULONG_ First,
    LONG_(BOOST_WINAPI_WINAPI_CC *Handler)(PEXCEPTION_POINTERS_)) {
  return ::AddVectoredExceptionHandler(
      First, reinterpret_cast<LONG_(BOOST_WINAPI_WINAPI_CC *)(
                 ::_EXCEPTION_POINTERS *)>(Handler));
}

BOOST_FORCEINLINE ULONG_ RemoveVectoredExceptionHandler(PVOID_ Handle) {
  return ::RemoveVectoredExceptionHandler(Handle);
}

} // namespace boost::winapi

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

namespace winapi = boost::winapi;

winapi::LONG_
vectoredExceptionHandler(winapi::EXCEPTION_POINTERS_ *ExceptionInfo) {
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

[[noreturn]] inline void Fault::emitFault(ErrCode Error) {
  assuming(localHandler != nullptr);
  longjmp(localHandler->Buffer, static_cast<int>(Error.operator uint32_t()));
}

} // namespace WasmEdge
