// SPDX-License-Identifier: Apache-2.0
#include "interpreter/interpreter.h"
#include "common/ast/module.h"
#include "common/ast/section.h"
#include "runtime/instance/module.h"
#include "support/log.h"

namespace SSVM {
namespace Interpreter {

/// Instantiate Wasm Module. See "include/interpreter/interpreter.h".
Expect<void> Interpreter::instantiateModule(Runtime::StoreManager &StoreMgr,
                                            const AST::Module &Mod,
                                            std::string_view Name) {
  InsMode = InstantiateMode::Instantiate;
  if (auto Res = instantiate(StoreMgr, Mod, Name); !Res) {
    if (Name != "") {
      LOG(ERROR) << ErrInfo::InfoRegistering(Name);
    }
    return Unexpect(Res);
  }
  return {};
}

/// Register host module. See "include/interpreter/interpreter.h".
Expect<void> Interpreter::registerModule(Runtime::StoreManager &StoreMgr,
                                         const Runtime::ImportObject &Obj) {
  StoreMgr.reset();
  /// Check is module name duplicated.
  if (auto Res = StoreMgr.findModule(Obj.getModuleName())) {
    LOG(ERROR) << ErrCode::ModuleNameConflict;
    LOG(ERROR) << ErrInfo::InfoRegistering(Obj.getModuleName());
    return Unexpect(ErrCode::ModuleNameConflict);
  }
  auto NewModInst =
      std::make_unique<Runtime::Instance::ModuleInstance>(Obj.getModuleName());
  auto ModInstAddr = StoreMgr.importModule(NewModInst);
  auto *ModInst = *StoreMgr.getModule(ModInstAddr);

  for (auto &Func : Obj.getFuncs()) {
    Func.second->setModuleAddr(ModInstAddr);
    uint32_t Addr = StoreMgr.importHostFunction(*Func.second.get());
    ModInst->addFuncAddr(Addr);
    ModInst->exportFunction(Func.first, ModInst->getFuncNum() - 1);
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
                                         std::string_view Name) {
  InsMode = InstantiateMode::ImportWasm;
  if (auto Res = instantiate(StoreMgr, Mod, Name); !Res) {
    LOG(ERROR) << ErrInfo::InfoRegistering(Name);
    return Unexpect(Res);
  }
  return {};
}

/// Invoke function. See "include/interpreter/interpreter.h".
Expect<std::vector<ValVariant>>
Interpreter::invoke(Runtime::StoreManager &StoreMgr, const uint32_t FuncAddr,
                    Span<const ValVariant> Params) {
  /// Check and get function address from store manager.
  Runtime::Instance::FunctionInstance *FuncInst;
  if (auto Res = StoreMgr.getFunction(FuncAddr)) {
    FuncInst = *Res;
  } else {
    return Unexpect(Res);
  }

  /// Check parameter and function type.
  const auto &FuncType = FuncInst->getFuncType();
  if (FuncType.Params.size() > Params.size()) {
    std::vector<ValType> GotParams;
    for (size_t I = 0; I < Params.size(); ++I) {
      GotParams.push_back(FuncType.Params[I]);
    }
    LOG(ERROR) << ErrCode::FuncSigMismatch;
    LOG(ERROR) << ErrInfo::InfoMismatch(FuncType.Params, FuncType.Returns,
                                        GotParams, FuncType.Returns);
    return Unexpect(ErrCode::FuncSigMismatch);
  } else if (FuncType.Params.size() < Params.size()) {
    std::vector<ValType> GotParams = FuncType.Params;
    for (size_t I = FuncType.Params.size(); I < Params.size(); ++I) {
      GotParams.push_back(ValType::I32);
    }
    LOG(ERROR) << ErrCode::FuncSigMismatch;
    LOG(ERROR) << ErrInfo::InfoMismatch(FuncType.Params, FuncType.Returns,
                                        GotParams, FuncType.Returns);
    return Unexpect(ErrCode::FuncSigMismatch);
  }

  /// Call runFunction.
  if (auto Res = runFunction(StoreMgr, *FuncInst, Params); !Res) {
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
