// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "system/stacktrace.h"
#include "common/spdlog.h"

#include "runtime/instance/module.h"
#include <cstdint>
#include <map>
#include <unordered_map>

#if WASMEDGE_OS_WINDOWS
#include "system/winapi.h"
#elif WASMEDGE_OS_LINUX
#include <unwind.h>
#elif WASMEDGE_OS_MACOS
#include <execinfo.h>
#endif

namespace WasmEdge {

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
        auto &BTState = *static_cast<BacktraceState *>(Arg);
        if (BTState.Index >= BTState.Buffer.size()) {
          return _URC_END_OF_STACK;
        }
        BTState.Buffer[BTState.Index++] =
            reinterpret_cast<void *>(_Unwind_GetIP(Ctx));
        return _URC_NO_REASON;
      },
      &State);
  return Buffer.first(State.Index);
#elif WASMEDGE_OS_MACOS
  const auto Depth = backtrace(Buffer.data(), Buffer.size());
  return Buffer.first(Depth);
#endif
}

Span<const StackTraceEntry>
interpreterStackTrace(const Runtime::StackManager &StackMgr,
                      Span<StackTraceEntry> Buffer) noexcept {
  size_t Index = 0;
  std::unordered_map<const Runtime::Instance::ModuleInstance *,
                     std::map<AST::InstrView::iterator, int64_t>>
      Cache;
  const auto Frames = StackMgr.getFramesSpan();
  for (size_t I = 1; I < Frames.size(); ++I) {
    // A native-entry frame carries the callee's own end iterator instead of a
    // return address in the caller, so it resolves against no module.
    if (Frames[I].NativeEntry) {
      continue;
    }
    const auto *Module = Frames[I - 1].Module;
    if (Module == nullptr) {
      continue;
    }
    auto [CacheIter, Inserted] = Cache.try_emplace(Module);
    auto &Funcs = CacheIter->second;
    if (Inserted) {
      const auto FuncInsts = Module->getFunctionInstances();
      for (size_t J = 0; J < FuncInsts.size(); ++J) {
        const auto &Func = FuncInsts[J];
        if (Func && Func->isWasmFunction()) {
          const auto &Instrs = Func->getInstrs();
          Funcs.emplace(Instrs.end(), INT64_C(-1));
          Funcs.emplace(Instrs.begin(), static_cast<int64_t>(J));
        }
      }
    }
    auto Entry = Frames[I].From;
    auto Iter = Funcs.lower_bound(Entry);
    if ((Iter == Funcs.end() || Iter->first > Entry) && Iter != Funcs.begin()) {
      --Iter;
    }
    if (Iter != Funcs.end() && Iter->first <= Entry &&
        Iter->second >= INT64_C(0) && Index < Buffer.size()) {
      Buffer[Index++] =
          StackTraceEntry{Module, static_cast<uint32_t>(Iter->second)};
    }
  }
  return Buffer.first(Index);
}

Span<const StackTraceEntry>
compiledStackTrace(Span<const Runtime::Instance::ModuleInstance *const> Modules,
                   Span<void *const> Stack,
                   Span<StackTraceEntry> Buffer) noexcept {
  struct FuncEntry {
    const Runtime::Instance::ModuleInstance *Module;
    int64_t Index;
  };
  // Known limitation: two instances of the same compiled module share their
  // code addresses, so emplace keeps whichever instance is enumerated first and
  // a trap in the other one is reported against it. A native frame carries no
  // instance identity to tell them apart.
  std::map<void *, FuncEntry> Funcs;
  for (const auto *Module : Modules) {
    if (Module == nullptr) {
      continue;
    }
    const auto FuncInsts = Module->getFunctionInstances();
    for (size_t I = 0; I < FuncInsts.size(); ++I) {
      const auto &Func = FuncInsts[I];
      if (Func && Func->isCompiledFunction() && Func->getModule() == Module) {
        Funcs.emplace(
            reinterpret_cast<void *>(Func->getFuncType().getSymbol().get()),
            FuncEntry{Module, INT64_C(-1)});
        Funcs.emplace(Func->getSymbol().get(),
                      FuncEntry{Module, static_cast<int64_t>(I)});
      }
    }
  }
  size_t Index = 0;
  for (auto Address : Stack) {
    auto Probe =
        reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(Address) - 1);
    auto Iter = Funcs.lower_bound(Probe);
    if ((Iter == Funcs.end() || Iter->first > Probe) && Iter != Funcs.begin()) {
      --Iter;
    }
    if (Iter != Funcs.end() && Iter->first < Probe &&
        Iter->second.Index >= INT64_C(0) && Index < Buffer.size()) {
      Buffer[Index++] = StackTraceEntry{
          Iter->second.Module, static_cast<uint32_t>(Iter->second.Index)};
    }
  }
  return Buffer.first(Index);
}

} // namespace WasmEdge
