// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

#include "llvm/lazyjit.h"

#include "ast/instruction.h"
#include "ast/module.h"
#include "common/spdlog.h"
#include "runtime/instance/module.h"
#include "llvm/compiler.h"
#include "llvm/data.h"
#include "llvm/jit.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace WasmEdge::LLVM {

namespace {

using namespace std::literals;

// Collect the not-yet-compiled local functions reachable from the seed
// function through direct calls and function references, so one lazy
// compilation batch covers the whole call graph of the entry.
std::vector<uint32_t> collectCallGraphBatch(
    uint32_t LocalSeed, const AST::Module &Module, uint32_t ImportFuncCount,
    const std::unordered_map<uint32_t, WasmFunctionCodeAddress> &Compiled) {
  std::vector<uint32_t> SortedLocals;
  const auto &CodeSec = Module.getCodeSection().getContent();
  const uint32_t DefinedCount = Module.getDefinedFuncCount();

  // The caller's findPendingCompile guarantees a valid, not-yet-compiled
  // seed.
  assuming(LocalSeed < DefinedCount && Compiled.count(LocalSeed) == 0);

  std::vector<uint8_t> Visited(DefinedCount, 0);
  std::vector<uint32_t> Stack;
  Stack.reserve(64);

  Visited[LocalSeed] = 1;
  Stack.push_back(LocalSeed);
  SortedLocals.push_back(LocalSeed);

  while (!Stack.empty()) {
    const uint32_t L = Stack.back();
    Stack.pop_back();

    for (const auto &Instr : CodeSec[L].getExpr().getInstrs()) {
      const auto Op = Instr.getOpCode();
      if (Op == OpCode::Call || Op == OpCode::Return_call ||
          Op == OpCode::Ref__func) {
        const uint32_t Target = Instr.getTargetIndex();
        if (Target >= ImportFuncCount) {
          const uint32_t LocalIdx = Target - ImportFuncCount;
          if (LocalIdx < DefinedCount && !Visited[LocalIdx] &&
              Compiled.count(LocalIdx) == 0) {
            Visited[LocalIdx] = 1;
            Stack.push_back(LocalIdx);
            SortedLocals.push_back(LocalIdx);
          }
        }
      }
    }
  }

  std::sort(SortedLocals.begin(), SortedLocals.end());
  return SortedLocals;
}

// Upgrade the function instance at GlobalFuncIdx of a bound module instance
// to run the compiled code at Address. Shared by the fresh-batch path and the
// re-instantiation restore path.
void upgradeToCompiled(
    Span<const Runtime::Instance::FunctionInstance *const> FuncInsts,
    size_t GlobalFuncIdx, JITLibrary &JITLib,
    WasmFunctionCodeAddress Address) noexcept {
  // A fully instantiated instance of the same AST module covers every
  // compiled local function index.
  assuming(GlobalFuncIdx < FuncInsts.size());
  // The function instances are owned mutable by the module instance; the
  // accessor only adds constness. Upgrading them to compiled mode is the
  // purpose of this engine. Non-wasm functions are declined by
  // unsafeUpgradeToCompiled itself.
  auto *FuncInst = const_cast<Runtime::Instance::FunctionInstance *>(
      FuncInsts[GlobalFuncIdx]);
  FuncInst->unsafeUpgradeToCompiled(JITLib.createCodeSymbol(Address));
}

// True while someone outside the engine still holds the AST module and could
// re-instantiate it; a state failing this can never be rebound, so keeping it
// would leak its JIT and compiled code for the lifetime of the engine.
bool isReinstantiable(
    const std::shared_ptr<const AST::Module> &Module) noexcept {
  return Module != nullptr && Module.use_count() > 1;
}

} // namespace

struct LazyJITEngine::Impl {
  struct ModuleState {
    /// Shared ownership of the AST module for on-demand compilation. Held
    /// from prepare time on, so the AST module pointers used as map keys
    /// below can never dangle or be reused by another allocation.
    std::shared_ptr<const AST::Module> Module;
    /// Per-module LLVM data holding the thread-safe context.
    Data LLData;
    /// Per-module ORC LLJIT holding the generated code.
    std::shared_ptr<JITLibrary> JITLib;
    /// Resolved machine code addresses of lazily compiled functions, keyed
    /// by local function index. Survives re-instantiations, so rebinding
    /// restores compiled functions without further JIT symbol lookups.
    std::unordered_map<uint32_t, WasmFunctionCodeAddress> CompiledCode;
    /// Reverse lookup from function instance to function index.
    std::unordered_map<const Runtime::Instance::FunctionInstance *, uint32_t>
        FuncIndices;
    /// Number of imported functions of the module.
    uint32_t ImportFuncCount = 0;
  };

  Impl(const Configure &C) noexcept : Conf(C) {}

  /// Locate the bound state and the local function index when the function
  /// still needs lazy compilation. Returns {nullptr, 0} when there is
  /// nothing to do. The caller must hold Mutex (shared or exclusive); all
  /// writers hold it exclusively, so shared-locked reads are race-free.
  std::pair<ModuleState *, uint32_t>
  findPendingCompile(const Runtime::Instance::ModuleInstance *ModInst,
                     const Runtime::Instance::FunctionInstance *FuncInst) {
    // Already compiled or not a wasm function: nothing to do. Checked first
    // because it needs no map lookup, so the steady state where every call
    // probes an already-compiled function short-circuits here. The check is
    // done under the engine mutex to avoid racing with the upgrade in
    // compileOnDemand.
    if (!FuncInst->isWasmFunction() || FuncInst->isCompiledFunction()) {
      return {nullptr, 0};
    }
    auto StateIt = States.find(ModInst);
    if (StateIt == States.end()) {
      return {nullptr, 0};
    }
    auto &State = StateIt->second;
    auto IdxIt = State.FuncIndices.find(FuncInst);
    if (IdxIt == State.FuncIndices.end()) {
      // A bound module knows all of its function instances; reaching here
      // indicates a foreign or stale instance.
      spdlog::debug(
          "[lazy-jit]: function instance not bound to its module state"sv);
      return {nullptr, 0};
    }
    const uint32_t FuncIdx = IdxIt->second;
    if (FuncIdx < State.ImportFuncCount) {
      return {nullptr, 0};
    }
    const uint32_t LocalFuncIdx = FuncIdx - State.ImportFuncCount;
    if (State.CompiledCode.count(LocalFuncIdx) > 0) {
      return {nullptr, 0};
    }
    return {&State, LocalFuncIdx};
  }

  const Configure Conf;
  mutable std::shared_mutex Mutex;
  /// States prepared but not yet bound to a module instance, keyed by the
  /// AST module owned by the state itself.
  std::unordered_map<const AST::Module *, ModuleState> PendingStates;
  /// States bound to instantiated module instances.
  std::unordered_map<const Runtime::Instance::ModuleInstance *, ModuleState>
      States;
};

LazyJITEngine::LazyJITEngine(const Configure &Conf) noexcept
    : PImpl(std::make_unique<Impl>(Conf)) {}

LazyJITEngine::~LazyJITEngine() noexcept = default;

Expect<std::shared_ptr<Executable>>
LazyJITEngine::prepare(std::shared_ptr<const AST::Module> Module) {
  if (!Module) {
    return Unexpect(ErrCode::Value::WrongVMWorkflow);
  }
  Impl::ModuleState State;
  State.Module = std::move(Module);

  Compiler InfraCompiler(PImpl->Conf);
  EXPECTED_TRY(InfraCompiler.checkConfigure());

  EXPECTED_TRY(State.LLData,
               InfraCompiler.compileInfrastructure(*State.Module));

  JIT JITEngine(PImpl->Conf);
  EXPECTED_TRY(auto Exec, JITEngine.loadLazy(State.LLData));
  State.JITLib = std::static_pointer_cast<JITLibrary>(Exec);

  State.ImportFuncCount = State.Module->getImportFuncCount();

  std::unique_lock Lock(PImpl->Mutex);
  // Prune pending states nobody can re-instantiate anymore.
  for (auto It = PImpl->PendingStates.begin();
       It != PImpl->PendingStates.end();) {
    if (isReinstantiable(It->second.Module)) {
      ++It;
    } else {
      It = PImpl->PendingStates.erase(It);
    }
  }
  const auto *Key = State.Module.get();
  PImpl->PendingStates.insert_or_assign(Key, std::move(State));
  return Exec;
}

void LazyJITEngine::registerInstance(
    const Runtime::Instance::ModuleInstance &ModInst,
    std::shared_ptr<const AST::Module> Module) noexcept {
  if (!Module) {
    return;
  }
  std::unique_lock Lock(PImpl->Mutex);
  auto It = PImpl->PendingStates.find(Module.get());
  if (It == PImpl->PendingStates.end()) {
    return;
  }
  auto State = std::move(It->second);
  PImpl->PendingStates.erase(It);

  const auto FuncInsts = ModInst.getFunctionInstances();
  State.FuncIndices.reserve(FuncInsts.size());
  for (uint32_t I = 0; I < FuncInsts.size(); ++I) {
    State.FuncIndices.emplace(FuncInsts[I], I);
  }

  // Rebinding after a re-instantiation: the fresh function instances start
  // in interpreter mode, so restore the functions already compiled in
  // earlier instantiations from their persisted code addresses.
  for (const auto &[LocalFuncIdx, Address] : State.CompiledCode) {
    upgradeToCompiled(FuncInsts, size_t{State.ImportFuncCount} + LocalFuncIdx,
                      *State.JITLib, Address);
  }

  PImpl->States.insert_or_assign(&ModInst, std::move(State));
}

void LazyJITEngine::unregisterInstance(
    const Runtime::Instance::ModuleInstance &ModInst) noexcept {
  std::unique_lock Lock(PImpl->Mutex);
  auto It = PImpl->States.find(&ModInst);
  if (It == PImpl->States.end()) {
    return;
  }
  // Drop only the per-instance bindings; the module-level JIT state moves
  // back to the pending map so a later instantiation of the same AST module
  // (which skips prepare because its executable is already hooked) rebinds
  // it instead of silently losing lazy compilation.
  auto State = std::move(It->second);
  PImpl->States.erase(It);
  State.FuncIndices.clear();
  // Keep the state only while it can be rebound; otherwise drop it instead
  // of leaking the JIT and its compiled code.
  if (const auto *Key = State.Module.get(); isReinstantiable(State.Module)) {
    PImpl->PendingStates.insert_or_assign(Key, std::move(State));
  }
}

Expect<void> LazyJITEngine::compileOnDemand(
    const Runtime::Instance::FunctionInstance *FuncInst) {
  if (FuncInst == nullptr) {
    return {};
  }
  const auto *ModInst = FuncInst->getModule();
  if (ModInst == nullptr) {
    return {};
  }

  // Fast path: the common no-work cases (unbound module, host function,
  // already compiled) need only read access. All writers hold the exclusive
  // lock on the same mutex, so the shared lock keeps this race-free without
  // serializing concurrent callers.
  {
    std::shared_lock SharedLock(PImpl->Mutex);
    if (PImpl->findPendingCompile(ModInst, FuncInst).first == nullptr) {
      return {};
    }
  }

  std::unique_lock Lock(PImpl->Mutex);
  // Re-locate under the exclusive lock; the state may have changed between
  // the two locks.
  auto [StatePtr, LocalFuncIdx] = PImpl->findPendingCompile(ModInst, FuncInst);
  if (StatePtr == nullptr) {
    return {};
  }
  auto &State = *StatePtr;

  auto BatchLocals = collectCallGraphBatch(
      LocalFuncIdx, *State.Module, State.ImportFuncCount, State.CompiledCode);

  spdlog::debug(
      "[lazy-jit]: lazy compiling batch ({} local funcs) for entry local {}"sv,
      BatchLocals.size(), LocalFuncIdx);

  const auto LogError = [](std::string_view Stage) {
    return [Stage](auto Err) {
      spdlog::error("[lazy-jit]: {} failed: {}"sv, Stage, Err);
      return Err;
    };
  };

  Compiler BatchCompiler(PImpl->Conf);
  EXPECTED_TRY(BatchCompiler.checkConfigure().map_error(
      LogError("lazy JIT compiler config"sv)));

  EXPECTED_TRY(
      auto CompiledData,
      BatchCompiler
          .compileFunctions(std::move(State.LLData), *State.Module, BatchLocals)
          .map_error(LogError("lazy JIT function compilation"sv)));
  State.LLData = std::move(CompiledData);

  std::vector<uint32_t> BatchGlobal;
  BatchGlobal.reserve(BatchLocals.size());
  for (uint32_t L : BatchLocals) {
    BatchGlobal.push_back(State.ImportFuncCount + L);
  }

  // The JIT library is created in prepare() and lives as long as the state.
  assuming(State.JITLib);
  JIT JITEngine(PImpl->Conf);
  EXPECTED_TRY(auto ResolvedAddresses,
               JITEngine.add(*State.JITLib, State.LLData, BatchGlobal)
                   .map_error(LogError("lazy JIT add"sv)));

  // The machine code now lives in the persisted JIT regardless of the
  // instance bindings, so record each address before upgrading its instance.
  const auto FuncInsts = ModInst->getFunctionInstances();
  for (size_t I = 0; I < BatchLocals.size(); ++I) {
    State.CompiledCode.emplace(BatchLocals[I], ResolvedAddresses[I]);
    upgradeToCompiled(FuncInsts, BatchGlobal[I], *State.JITLib,
                      ResolvedAddresses[I]);
  }

  spdlog::debug(
      "[lazy-jit]: lazy compilation completed for batch of {} functions, "
      "total compiled: {}"sv,
      BatchLocals.size(), State.CompiledCode.size());

  return {};
}

uint32_t LazyJITEngine::compiledFunctionCount() const noexcept {
  std::shared_lock Lock(PImpl->Mutex);
  uint32_t Count = 0;
  for (const auto &Pair : PImpl->States) {
    Count += static_cast<uint32_t>(Pair.second.CompiledCode.size());
  }
  // Pending states of unbound modules still hold live compiled code.
  for (const auto &Pair : PImpl->PendingStates) {
    Count += static_cast<uint32_t>(Pair.second.CompiledCode.size());
  }
  return Count;
}

void LazyJITEngine::clear() noexcept {
  std::unique_lock Lock(PImpl->Mutex);
  PImpl->PendingStates.clear();
  PImpl->States.clear();
}

} // namespace WasmEdge::LLVM
