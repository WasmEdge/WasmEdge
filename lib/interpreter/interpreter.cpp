// SPDX-License-Identifier: Apache-2.0
#include "interpreter/interpreter.h"
#include "common/ast/module.h"
#include "common/ast/section.h"
#include "runtime/instance/module.h"
#include "support/log.h"

namespace SSVM {
namespace Interpreter {

namespace {
static void loggingErrorInst(ErrCode Code) {
  /// TODO: Refine error logging.
  LOG(ERROR) << "Instantiation failed: " << ErrStr[(uint32_t)Code]
             << ", Code: 0x" << std::hex << (uint32_t)Code << std::dec;
}

static void loggingErrorExec(ErrCode Code) {
  /// TODO: Refine error logging.
  LOG(ERROR) << "Execution failed: " << ErrStr[(uint32_t)Code] << ", Code: 0x"
             << std::hex << (uint32_t)Code << std::dec;
}
} // namespace

/// Instantiate Wasm Module. See "include/interpreter/interpreter.h".
Expect<void> Interpreter::instantiateModule(Runtime::StoreManager &StoreMgr,
                                            const AST::Module &Mod,
                                            const std::string &Name) {
  InsMode = InstantiateMode::Instantiate;
  if (auto Res = instantiate(StoreMgr, Mod, Name); !Res) {
    loggingErrorInst(Res.error());
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
    loggingErrorInst(ErrCode::ModuleNameConflict);
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
  if (auto Res = instantiate(StoreMgr, Mod, Name); !Res) {
    loggingErrorInst(Res.error());
    return Unexpect(Res);
  }
  return {};
}

/// Invoke function. See "include/interpreter/interpreter.h".
Expect<std::vector<ValVariant>>
Interpreter::invoke(Runtime::StoreManager &StoreMgr, const uint32_t FuncAddr,
                    const std::vector<ValVariant> &Params) {
  /// Check and get function address from store manager.
  Runtime::Instance::FunctionInstance *FuncInst;
  if (auto Res = StoreMgr.getFunction(FuncAddr)) {
    FuncInst = *Res;
  } else {
    loggingErrorExec(Res.error());
    return Unexpect(Res);
  }

  /// Check parameter and function type.
  const auto &FuncType = FuncInst->getFuncType();
  if (FuncType.Params.size() != Params.size()) {
    loggingErrorExec(ErrCode::FuncSigMismatch);
    return Unexpect(ErrCode::FuncSigMismatch);
  }

  /// Call runFunction.
  if (auto Res = runFunction(StoreMgr, *FuncInst, Params); !Res) {
    loggingErrorExec(Res.error());
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
