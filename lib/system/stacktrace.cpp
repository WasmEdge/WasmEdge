// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "system/stacktrace.h"
#include "common/spdlog.h"
#include <fmt/ranges.h>

#if WASMEDGE_OS_WINDOWS
#include "system/winapi.h"
#elif WASMEDGE_OS_LINUX
#include <unwind.h>
#elif WASMEDGE_OS_MACOS || WASMEDGE_OS_FREEBSD
#include <execinfo.h>
#endif

namespace WasmEdge {

using namespace std::literals;

Span<void *const> stackTrace(Span<void *> Buffer) noexcept {
#if WASMEDGE_OS_WINDOWS
  struct DbgHelp {
    DbgHelp() noexcept : Process(winapi::GetCurrentProcess()) {
      winapi::SymSetOptions(winapi::SYMOPT_DEFERRED_LOADS_);
      winapi::SymInitializeW(Process, nullptr, true);
      SelfBase = winapi::SymGetModuleBase64(
          Process, reinterpret_cast<winapi::DWORD64_>(&stackTrace));
      NtDllBase = winapi::SymGetModuleBase64(
          Process, reinterpret_cast<winapi::DWORD64_>(
                       &winapi::RtlCaptureStackBackTrace));
      Kernel32Base = winapi::SymGetModuleBase64(
          Process, reinterpret_cast<winapi::DWORD64_>(&winapi::CloseHandle));
    }
    ~DbgHelp() noexcept { winapi::SymCleanup(Process); }
    void refresh() noexcept { winapi::SymRefreshModuleList(Process); }
    winapi::HANDLE_ Process;
    winapi::DWORD64_ SelfBase, NtDllBase, Kernel32Base;
  };
  static DbgHelp Helper;
  Helper.refresh();
  auto Depth = static_cast<size_t>(winapi::RtlCaptureStackBackTrace(
      1u, static_cast<winapi::ULONG_>(Buffer.size()), Buffer.data(), nullptr));
  size_t NewDepth = 0;
  for (size_t I = 0; I < Depth; ++I) {
    auto Base = winapi::SymGetModuleBase64(
        Helper.Process, reinterpret_cast<winapi::DWORD64_>(Buffer[I]));
    if (Base == 0 || (Base != Helper.SelfBase && Base != Helper.NtDllBase &&
                      Base != Helper.Kernel32Base)) {
      Buffer[NewDepth++] = Buffer[I];
    }
  }
  return Buffer.first(static_cast<size_t>(NewDepth));
#elif WASMEDGE_OS_LINUX
  struct BacktraceState {
    Span<void *> Buffer;
    size_t Index;
  };
  BacktraceState State{Buffer, 0};
  _Unwind_Backtrace(
      [](struct _Unwind_Context *Ctx, void *Arg) noexcept {
        auto &State = *static_cast<BacktraceState *>(Arg);
        if (State.Index >= State.Buffer.size()) {
          return _URC_END_OF_STACK;
        }
        State.Buffer[State.Index++] =
            reinterpret_cast<void *>(_Unwind_GetIP(Ctx));
        return _URC_NO_REASON;
      },
      &State);
  return Buffer.first(State.Index);
#elif WASMEDGE_OS_MACOS || WASMEDGE_OS_FREEBSD
  const auto Depth = backtrace(Buffer.data(), Buffer.size());
  return Buffer.first(Depth);
#endif
}

Span<const uint32_t>
interpreterStackTrace(const Runtime::StackManager &StackMgr,
                      Span<uint32_t> Buffer) noexcept {
  size_t Index = 0;
  if (auto Module = StackMgr.getModule()) {
    const auto FuncInsts = Module->getFunctionInstances();
    std::map<AST::InstrView::iterator, int64_t> Funcs;
    for (size_t I = 0; I < FuncInsts.size(); ++I) {
      const auto &Func = FuncInsts[I];
      if (Func && Func->isWasmFunction()) {
        const auto &Instrs = Func->getInstrs();
        Funcs.emplace(Instrs.end(), INT64_C(-1));
        Funcs.emplace(Instrs.begin(), I);
      }
    }
    for (const auto &Frame : StackMgr.getFramesSpan()) {
      auto Entry = Frame.From;
      auto Iter = Funcs.lower_bound(Entry);
      if ((Iter == Funcs.end() || Iter->first > Entry) &&
          Iter != Funcs.begin()) {
        --Iter;
      }
      if (Iter != Funcs.end() && Iter->first < Entry &&
          Iter->second >= INT64_C(0) && Index < Buffer.size()) {
        Buffer[Index++] = static_cast<uint32_t>(Iter->second);
      }
    }
  }
  return Buffer.first(Index);
}

Span<const uint32_t> compiledStackTrace(const Runtime::StackManager &StackMgr,
                                        Span<uint32_t> Buffer) noexcept {
  std::array<void *, 256> StackTraceBuffer;
  return compiledStackTrace(StackMgr, stackTrace(StackTraceBuffer), Buffer);
}

Span<const uint32_t> compiledStackTrace(const Runtime::StackManager &StackMgr,
                                        Span<void *const> Stack,
                                        Span<uint32_t> Buffer) noexcept {
  std::map<void *, int64_t> Funcs;
  size_t Index = 0;
  if (auto Module = StackMgr.getModule()) {
    const auto FuncInsts = Module->getFunctionInstances();
    for (size_t I = 0; I < FuncInsts.size(); ++I) {
      const auto &Func = FuncInsts[I];
      if (Func && Func->isCompiledFunction()) {
        Funcs.emplace(
            reinterpret_cast<void *>(Func->getFuncType().getSymbol().get()),
            INT64_C(-1));
        Funcs.emplace(Func->getSymbol().get(), I);
      }
    }
    for (auto Entry : Stack) {
      auto Iter = Funcs.lower_bound(Entry);
      if ((Iter == Funcs.end() || Iter->first > Entry) &&
          Iter != Funcs.begin()) {
        --Iter;
      }
      if (Iter != Funcs.end() && Iter->first < Entry &&
          Iter->second >= INT64_C(0) && Index < Buffer.size()) {
        Buffer[Index++] = static_cast<uint32_t>(Iter->second);
      }
    }
  }
  return Buffer.first(Index);
}

void dumpStackTrace(Span<const uint32_t> Stack) noexcept {
  spdlog::error("calling stack:{}"sv, fmt::join(Stack, ", "sv));
}

} // namespace WasmEdge
