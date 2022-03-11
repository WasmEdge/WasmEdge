// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/log.h"

namespace WasmEdge {
namespace Executor {

// Instantiate Wasm Module. See "include/executor/executor.h".
Expect<void> Executor::instantiateModule(Runtime::StoreManager &StoreMgr,
                                         const AST::Module &Mod) {
  InsMode = InstantiateMode::Instantiate;
  if (auto Res = instantiate(StoreMgr, Mod, ""); !Res) {
    // If Statistics is enabled, then dump it here.
    // When there is an error happened, the following execution will not
    // execute.
    if (Stat) {
      Stat->dumpToLog(Conf);
    }
    return Unexpect(Res);
  }
  return {};
}

// Register host module. See "include/executor/executor.h".
Expect<void> Executor::registerModule(Runtime::StoreManager &StoreMgr,
                                      const Runtime::ImportObject &Obj) {
  StoreMgr.reset();
  // Check is module name duplicated.
  if (auto Res = StoreMgr.findModule(Obj.getModuleName())) {
    spdlog::error(ErrCode::ModuleNameConflict);
    spdlog::error(ErrInfo::InfoRegistering(Obj.getModuleName()));
    return Unexpect(ErrCode::ModuleNameConflict);
  }
  auto *ModInst = StoreMgr.importModule(Obj.getModuleName());

  for (auto &Func : Obj.getFuncs()) {
    auto *Inst = StoreMgr.importHostFunction(*Func.second.get());
    ModInst->addFunc(Inst);
    ModInst->exportFunction(Func.first, ModInst->getFuncNum() - 1);
  }
  for (auto &Tab : Obj.getTables()) {
    auto *Inst = StoreMgr.importHostTable(*Tab.second.get());
    ModInst->addTable(Inst);
    ModInst->exportTable(Tab.first, ModInst->getTableNum() - 1);
  }
  for (auto &Mem : Obj.getMems()) {
    auto *Inst = StoreMgr.importHostMemory(*Mem.second.get());
    ModInst->addMemory(Inst);
    ModInst->exportMemory(Mem.first, ModInst->getMemNum() - 1);
  }
  for (auto &Glob : Obj.getGlobals()) {
    auto *Inst = StoreMgr.importHostGlobal(*Glob.second.get());
    ModInst->addGlobal(Inst);
    ModInst->exportGlobal(Glob.first, ModInst->getGlobalNum() - 1);
  }
  return {};
}

// Register Wasm module. See "include/executor/executor.h".
Expect<void> Executor::registerModule(Runtime::StoreManager &StoreMgr,
                                      const AST::Module &Mod,
                                      std::string_view Name) {
  InsMode = InstantiateMode::ImportWasm;
  if (auto Res = instantiate(StoreMgr, Mod, Name); !Res) {
    spdlog::error(ErrInfo::InfoRegistering(Name));
    // If Statistics is enabled, then dump it here.
    // When there is an error happened, the following execution will not
    // execute.
    if (Stat) {
      Stat->dumpToLog(Conf);
    }
    return Unexpect(Res);
  }
  return {};
}

// Invoke function. See "include/executor/executor.h".
Expect<std::vector<std::pair<ValVariant, ValType>>>
Executor::invoke(Runtime::StoreManager &StoreMgr,
                 const Runtime::Instance::FunctionInstance &FuncInst,
                 Span<const ValVariant> Params,
                 Span<const ValType> ParamTypes) {
  // Check parameter and function type.
  const auto &FuncType = FuncInst.getFuncType();
  const auto &PTypes = FuncType.getParamTypes();
  const auto &RTypes = FuncType.getReturnTypes();
  std::vector<ValType> GotParamTypes(ParamTypes.begin(), ParamTypes.end());
  GotParamTypes.resize(Params.size(), ValType::I32);
  if (PTypes != GotParamTypes) {
    spdlog::error(ErrCode::FuncSigMismatch);
    spdlog::error(ErrInfo::InfoMismatch(PTypes, RTypes, GotParamTypes, RTypes));
    return Unexpect(ErrCode::FuncSigMismatch);
  }

  Runtime::StackManager StackMgr;

  // Call runFunction.
  if (auto Res = runFunction(StoreMgr, StackMgr, FuncInst, Params); !Res) {
    return Unexpect(Res);
  }

  // Get return values.
  std::vector<std::pair<ValVariant, ValType>> Returns(RTypes.size());
  for (uint32_t I = 0; I < RTypes.size(); ++I) {
    Returns[RTypes.size() - I - 1] =
        std::make_pair(StackMgr.pop(), RTypes[RTypes.size() - I - 1]);
  }
  return Returns;
}

} // namespace Executor
} // namespace WasmEdge
