// SPDX-License-Identifier: Apache-2.0
#include "interpreter/interpreter.h"
#include "common/ast/module.h"
#include "common/ast/section.h"
#include "executor/instance/module.h"
#include "support/log.h"

#include <boost/algorithm/hex.hpp>
#include <boost/format.hpp>
#include <cstdlib>

namespace SSVM {
namespace Interpreter {

/// Instantiate Wasm Module. See "include/interpreter/interpreter.h".
Expect<void> Interpreter::instantiateModule(Runtime::StoreManager &StoreMgr,
                                            const AST::Module &Mod,
                                            const std::string &Name) {
  InsMode = InstantiateMode::Instantiate;
  return instantiate(StoreMgr, Mod, Name);
}

/// Register host module. See "include/interpreter/interpreter.h".
Expect<void> Interpreter::registerModule(Runtime::StoreManager &StoreMgr,
                                         const Runtime::ImportObject &Obj) {
  StoreMgr.reset();
  /// Check is module name duplicated.
  if (auto Res = StoreMgr.findModule(Obj.getModuleName())) {
    return Unexpect(ErrCode::ModuleNameConflict);
  }
  auto NewModInst =
      std::make_unique<Runtime::Instance::ModuleInstance>(Obj.getModuleName());
  auto ModInstAddr = StoreMgr.importModule(NewModInst);
  auto *ModInst = *StoreMgr.getModule(ModInstAddr);

  for (auto &Func : Obj.getFuncs()) {
    uint32_t Addr = StoreMgr.importHostFunction(*Func.second.get());
    ModInst->addFuncAddr(Addr);
    ModInst->exportFuncion(Func.first, ModInst->getFuncNum() - 1);
  }
  for (auto &Tab : Obj.getTables()) {
    uint32_t Addr = StoreMgr.importHostTable(*Tab.second.get());
    ModInst->addTableAddr(Addr);
    ModInst->exportTable(Tab.first, ModInst->getTableNum() - 1);
  }
  for (auto &Mem : Obj.getMems()) {
    uint32_t Addr = StoreMgr.importHostMemory(*Mem.second.get());
    ModInst->addMemAddr(Addr);
    ModInst->exportMemory(Mem.first, ModInst->getMemNum() - 1);
  }
  for (auto &Glob : Obj.getGlobals()) {
    uint32_t Addr = StoreMgr.importHostGlobal(*Glob.second.get());
    ModInst->addGlobalAddr(Addr);
    ModInst->exportGlobal(Glob.first, ModInst->getGlobalNum() - 1);
  }
  return {};
}

/// Register Wasm module. See "include/interpreter/interpreter.h".
Expect<void> Interpreter::registerModule(Runtime::StoreManager &StoreMgr,
                                         const AST::Module &Mod,
                                         const std::string &Name = "") {
  InsMode = InstantiateMode::ImportWasm;
  return instantiate(StoreMgr, Mod, Name);
}

/// Invoke function. See "include/interpreter/interpreter.h".
Expect<std::vector<ValVariant>>
Interpreter::invoke(Runtime::StoreManager &StoreMgr, const std::string &Name,
                    const std::vector<ValVariant> &Params) {
  /// Check exports for finding function instance.
  const auto FuncExp = StoreMgr.getFuncExports();
  if (FuncExp.find(Name) == FuncExp.cend()) {
    return Unexpect(ErrCode::CallFunctionError);
  }
  const auto *FuncInst = *StoreMgr.getFunction(FuncExp.find(Name)->second);

  /// Check parameter and function type.
  const auto &FuncType = FuncInst->getFuncType();
  if (FuncType.Params.size() != Params.size()) {
    return Unexpect(ErrCode::TypeNotMatch);
  }

  /// Push arguments.
  InstrPdr.reset();
  StackMgr.reset();
  for (auto &Val : Params) {
    StackMgr.push(Val);
  }

  /// Call runFunction.
  if (auto Res = runFunction(StoreMgr, *FuncInst); !Res) {
    return Unexpect(Res);
  }

  /// Get return values.
  std::vector<ValVariant> Returns;
  for (uint32_t I = 0; I < FuncType.Returns.size(); ++I) {
    Returns.emplace_back(StackMgr.pop());
  }
  std::reverse(Returns.begin(), Returns.end());
  return Returns;
}

} // namespace Interpreter
} // namespace SSVM
