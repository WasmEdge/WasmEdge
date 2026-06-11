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
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace WasmEdge::LLVM {

namespace {

using namespace std::literals;

// Collect the not-yet-compiled local functions reachable from the seed
// function through direct calls and function references, so one lazy
// compilation batch covers the whole call graph of the entry.
std::vector<uint32_t>
collectCallGraphBatch(uint32_t LocalSeed, const AST::Module &Module,
                      uint32_t ImportFuncCount,
                      const std::unordered_set<uint32_t> &LazyCompiled) {
  std::vector<uint32_t> SortedLocals;
  const auto &CodeSec = Module.getCodeSection().getContent();
  const uint32_t DefinedCount = static_cast<uint32_t>(CodeSec.size());

  if (LocalSeed >= DefinedCount || LazyCompiled.count(LocalSeed) > 0) {
    return SortedLocals;
  }

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
              LazyCompiled.count(LocalIdx) == 0) {
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

uint32_t countImportFunctions(const AST::Module &Module) noexcept {
  uint32_t Count = 0;
  for (const auto &ImpDesc : Module.getImportSection().getContent()) {
    if (ImpDesc.getExternalType() == ExternalType::Function) {
      ++Count;
    }
  }
  return Count;
}

} // namespace

struct LazyJITEngine::Impl {
  struct ModuleState {
    /// Shared ownership of the AST module for on-demand compilation.
    std::shared_ptr<const AST::Module> Module;
    /// Per-module LLVM data holding the thread-safe context.
    Data LLData;
    /// Per-module ORC LLJIT holding the generated code.
    std::shared_ptr<JITLibrary> JITLib;
    /// Local indices of functions already lazily compiled.
    std::unordered_set<uint32_t> LazyCompiledFuncs;
    /// Function instances of the bound module instance by function index.
    std::vector<Runtime::Instance::FunctionInstance *> FuncInsts;
    /// Reverse lookup from function instance to function index.
    std::unordered_map<const Runtime::Instance::FunctionInstance *, uint32_t>
        FuncIndices;
    /// Number of imported functions of the module.
    uint32_t ImportFuncCount = 0;
  };

  Impl(const Configure &C) noexcept : Conf(C) {}

  const Configure Conf;
  mutable std::shared_mutex Mutex;
  /// States prepared but not yet bound to a module instance.
  std::unordered_map<const AST::Module *, ModuleState> PendingStates;
  /// States bound to instantiated module instances.
  std::unordered_map<const Runtime::Instance::ModuleInstance *, ModuleState>
      States;
};

LazyJITEngine::LazyJITEngine(const Configure &Conf) noexcept
    : PImpl(std::make_unique<Impl>(Conf)) {}

LazyJITEngine::~LazyJITEngine() noexcept = default;

Expect<std::shared_ptr<Executable>>
LazyJITEngine::prepare(const AST::Module &Module) {
  Impl::ModuleState State;

  Compiler InfraCompiler(PImpl->Conf);
  EXPECTED_TRY(InfraCompiler.checkConfigure());

  EXPECTED_TRY(State.LLData, InfraCompiler.compileInfrastructure(Module));

  JIT JITEngine(PImpl->Conf);
  EXPECTED_TRY(auto Exec, JITEngine.loadLazy(State.LLData));
  State.JITLib = std::static_pointer_cast<JITLibrary>(Exec);

  State.ImportFuncCount = countImportFunctions(Module);

  std::unique_lock Lock(PImpl->Mutex);
  PImpl->PendingStates.insert_or_assign(&Module, std::move(State));
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
  State.Module = std::move(Module);

  const auto FuncInsts = ModInst.getFunctionInstances();
  State.FuncInsts.reserve(FuncInsts.size());
  for (uint32_t I = 0; I < FuncInsts.size(); ++I) {
    // The function instances are owned mutable by the module instance; the
    // accessor only adds constness. Upgrading them to compiled mode is the
    // purpose of this engine.
    auto *FuncInst =
        const_cast<Runtime::Instance::FunctionInstance *>(FuncInsts[I]);
    State.FuncInsts.push_back(FuncInst);
    State.FuncIndices.emplace(FuncInst, I);
  }
  PImpl->States.insert_or_assign(&ModInst, std::move(State));
}

void LazyJITEngine::unregisterInstance(
    const Runtime::Instance::ModuleInstance &ModInst) noexcept {
  std::unique_lock Lock(PImpl->Mutex);
  PImpl->States.erase(&ModInst);
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

  std::unique_lock Lock(PImpl->Mutex);
  auto StateIt = PImpl->States.find(ModInst);
  if (StateIt == PImpl->States.end()) {
    return {};
  }
  auto &State = StateIt->second;

  auto IdxIt = State.FuncIndices.find(FuncInst);
  if (IdxIt == State.FuncIndices.end()) {
    return {};
  }
  const uint32_t FuncIdx = IdxIt->second;
  if (FuncIdx < State.ImportFuncCount) {
    return {};
  }
  const uint32_t LocalFuncIdx = FuncIdx - State.ImportFuncCount;

  // Already compiled or not a wasm function: nothing to do. This check is
  // done under the engine lock to avoid racing with the upgrade below.
  if (!FuncInst->isWasmFunction() || FuncInst->isCompiledFunction()) {
    return {};
  }
  if (State.LazyCompiledFuncs.count(LocalFuncIdx) > 0) {
    return {};
  }

  auto BatchLocals =
      collectCallGraphBatch(LocalFuncIdx, *State.Module, State.ImportFuncCount,
                            State.LazyCompiledFuncs);
  if (BatchLocals.empty()) {
    return {};
  }

  spdlog::debug(
      "[lazy-jit]: lazy compiling batch ({} local funcs) for entry local {}"sv,
      BatchLocals.size(), LocalFuncIdx);

  Compiler BatchCompiler(PImpl->Conf);
  EXPECTED_TRY(BatchCompiler.checkConfigure().map_error([](auto Err) {
    spdlog::error("[lazy-jit]: lazy JIT compiler config failed: {}"sv, Err);
    return Err;
  }));

  EXPECTED_TRY(
      auto CompiledData,
      BatchCompiler
          .compileFunctions(std::move(State.LLData), *State.Module, BatchLocals)
          .map_error([](auto Err) {
            spdlog::error(
                "[lazy-jit]: lazy JIT function compilation failed: {}"sv, Err);
            return Err;
          }));
  State.LLData = std::move(CompiledData);

  if (!State.JITLib) {
    spdlog::error("[lazy-jit]: missing JIT library for lazy compilation"sv);
    return Unexpect(ErrCode::Value::LazyCompilationError);
  }

  std::vector<uint32_t> BatchGlobal;
  BatchGlobal.reserve(BatchLocals.size());
  for (uint32_t L : BatchLocals) {
    BatchGlobal.push_back(State.ImportFuncCount + L);
  }

  JIT JITEngine(PImpl->Conf);
  EXPECTED_TRY(auto ResolvedAddresses,
               JITEngine.add(*State.JITLib, State.LLData, BatchGlobal)
                   .map_error([](auto Err) {
                     spdlog::error("[lazy-jit]: lazy JIT add failed: {}"sv,
                                   Err);
                     return Err;
                   }));

  for (size_t I = 0; I < BatchLocals.size(); ++I) {
    const uint32_t GlobalFuncIdx = State.ImportFuncCount + BatchLocals[I];
    if (GlobalFuncIdx >= State.FuncInsts.size()) {
      spdlog::error("[lazy-jit]: function index {} out of instance range"sv,
                    GlobalFuncIdx);
      return Unexpect(ErrCode::Value::WrongInstanceAddress);
    }
    auto *BatchFuncInst = State.FuncInsts[GlobalFuncIdx];
    if (BatchFuncInst->isWasmFunction()) {
      auto CompiledSym = State.JITLib->createCodeSymbol(ResolvedAddresses[I]);
      BatchFuncInst->unsafeUpgradeToCompiled(std::move(CompiledSym));
    }
  }

  State.LazyCompiledFuncs.insert(BatchLocals.begin(), BatchLocals.end());

  spdlog::debug(
      "[lazy-jit]: lazy compilation completed for batch of {} functions, "
      "total compiled: {}"sv,
      BatchLocals.size(), State.LazyCompiledFuncs.size());

  return {};
}

uint32_t LazyJITEngine::compiledFunctionCount() const noexcept {
  std::shared_lock Lock(PImpl->Mutex);
  uint32_t Count = 0;
  for (const auto &Pair : PImpl->States) {
    Count += static_cast<uint32_t>(Pair.second.LazyCompiledFuncs.size());
  }
  return Count;
}

void LazyJITEngine::clear() noexcept {
  std::unique_lock Lock(PImpl->Mutex);
  PImpl->PendingStates.clear();
  PImpl->States.clear();
}

} // namespace WasmEdge::LLVM
