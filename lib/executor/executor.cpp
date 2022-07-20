// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/log.h"

#include <iostream>

namespace WasmEdge {
namespace Executor {

/// Instantiate a WASM Module. See "include/executor/executor.h".
Expect<std::unique_ptr<Runtime::Instance::ModuleInstance>>
Executor::instantiateModule(Runtime::StoreManager &StoreMgr,
                            const AST::Module &Mod) {
  if (auto Res = instantiate(StoreMgr, Mod)) {
    return Res;
  } else {
    // If Statistics is enabled, then dump it here.
    // When there is an error happened, the following execution will not
    // execute.
    if (Stat) {
      Stat->dumpToLog(Conf);
    }
    return Unexpect(Res);
  }
}

/// Register a named WASM module. See "include/executor/executor.h".
Expect<std::unique_ptr<Runtime::Instance::ModuleInstance>>
Executor::registerModule(Runtime::StoreManager &StoreMgr,
                         const AST::Module &Mod, std::string_view Name) {
  if (auto Res = instantiate(StoreMgr, Mod, Name)) {
    return Res;
  } else {
    // If Statistics is enabled, then dump it here.
    // When there is an error happened, the following execution will not
    // execute.
    if (Stat) {
      Stat->dumpToLog(Conf);
    }
    return Unexpect(Res);
  }
}

/// Register an instantiated module. See "include/executor/executor.h".
Expect<void>
Executor::registerModule(Runtime::StoreManager &StoreMgr,
                         const Runtime::Instance::ModuleInstance &ModInst) {
  if (auto Res = StoreMgr.registerModule(&ModInst); !Res) {
    spdlog::error(ErrCode::Value::ModuleNameConflict);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(ErrCode::Value::ModuleNameConflict);
  }
  return {};
}

// Invoke function. See "include/executor/executor.h".
Expect<std::vector<std::pair<ValVariant, ValType>>>
Executor::invoke(const Runtime::Instance::FunctionInstance &FuncInst,
                 Span<const ValVariant> Params,
                 Span<const ValType> ParamTypes) {
  // Check parameter and function type.
  const auto &FuncType = FuncInst.getFuncType();
  const auto &PTypes = FuncType.getParamTypes();
  const auto &RTypes = FuncType.getReturnTypes();
  std::vector<ValType> GotParamTypes(ParamTypes.begin(), ParamTypes.end());
  GotParamTypes.resize(Params.size(), ValType::I32);
  if (PTypes != GotParamTypes) {
    spdlog::error(ErrCode::Value::FuncSigMismatch);
    spdlog::error(ErrInfo::InfoMismatch(PTypes, RTypes, GotParamTypes, RTypes));
    return Unexpect(ErrCode::Value::FuncSigMismatch);
  }

  Runtime::StackManager StackMgr;

  // Call runFunction.
  if (auto Res = runFunction(StackMgr, FuncInst, Params); !Res) {
    return Unexpect(Res);
  }

  // Get return values.
  std::vector<std::pair<ValVariant, ValType>> Returns(RTypes.size());
  for (uint32_t I = 0; I < RTypes.size(); ++I) {
    Returns[RTypes.size() - I - 1] =
        std::make_pair(StackMgr.pop(), RTypes[RTypes.size() - I - 1]);
  }

  // After execution, the value stack size should be 0.
  assuming(StackMgr.size() == 0);
  return Returns;
}

Expect<void> Executor::createThreadWithFunctionAddress(uint32_t FuncAddress) {
  std::cerr << "createThreadWithFunctionAddress " << FuncAddress << "\n";

  Runtime::StackManager &OriginalStackMgr = *CurrentStack;
  const Runtime::Instance::FunctionInstance &OriginalFuncInst =
      *(ExecutionContext.FuncInst);
  const AST::Instruction &Instr = *OriginalFuncInst.getInstrs().begin();

  // Get Table Instance
  const auto *TabInst =
      getTabInstByIdx(OriginalStackMgr, Instr.getSourceIndex());

  // If idx not small than tab.elem, trap.
  if (FuncAddress >= TabInst->getSize()) {
    spdlog::error(ErrCode::UndefinedElement);
    spdlog::error(ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset(),
                                           {FuncAddress},
                                           {ValTypeFromType<uint32_t>()}));
    return Unexpect(ErrCode::UndefinedElement);
  }

  ValVariant Ref = TabInst->getRefAddr(FuncAddress)->get<UnknownRef>();
  if (isNullRef(Ref)) {
    spdlog::error(ErrCode::UninitializedElement);
    spdlog::error(ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset(),
                                           {FuncAddress},
                                           {ValTypeFromType<uint32_t>()}));
    return Unexpect(ErrCode::UninitializedElement);
  }

  // Check function type.
  const auto *FuncInst = retrieveFuncRef(Ref);
  Runtime::StackManager StackMgr;

  Span<const ValVariant> Params;
  // Call runFunction.
  if (auto Res = runFunction(StackMgr, *FuncInst, Params); !Res) {
    return Unexpect(Res);
  }

  return {};
}

} // namespace Executor
} // namespace WasmEdge
