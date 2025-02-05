// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"
#include "system/stacktrace.h"

using namespace std::literals;

namespace WasmEdge {
namespace Executor {

Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
Executor::instantiateComponent(Runtime::StoreManager &StoreMgr,
                               const AST::Component::Component &Comp) {
  return instantiate(StoreMgr, Comp);
}
Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
Executor::instantiateComponent(Runtime::StoreManager &StoreMgr,
                               const AST::Component::Component &Comp,
                               std::string_view Name) {
  return instantiate(StoreMgr, Comp, Name);
}

/// Instantiate a WASM Module. See "include/executor/executor.h".
Expect<std::unique_ptr<Runtime::Instance::ModuleInstance>>
Executor::instantiateModule(Runtime::StoreManager &StoreMgr,
                            const AST::Module &Mod) {
  return instantiate(StoreMgr, Mod).map_error([this](auto E) {
    // If Statistics is enabled, then dump it here.
    // When there is an error happened, the following execution will not
    // execute.
    if (Stat) {
      Stat->dumpToLog(Conf);
    }
    return E;
  });
}

/// Register a named WASM module. See "include/executor/executor.h".
Expect<std::unique_ptr<Runtime::Instance::ModuleInstance>>
Executor::registerModule(Runtime::StoreManager &StoreMgr,
                         const AST::Module &Mod, std::string_view Name) {
  return instantiate(StoreMgr, Mod, Name).map_error([this](auto E) {
    // If Statistics is enabled, then dump it here.
    // When there is an error happened, the following execution will not
    // execute.
    if (Stat) {
      Stat->dumpToLog(Conf);
    }
    return E;
  });
}

/// Register an instantiated module. See "include/executor/executor.h".
Expect<void>
Executor::registerModule(Runtime::StoreManager &StoreMgr,
                         const Runtime::Instance::ModuleInstance &ModInst) {
  return StoreMgr.registerModule(&ModInst).map_error([](auto E) {
    E = ErrCode::Value::ModuleNameConflict;
    spdlog::error(E);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return E;
  });
}
Expect<void> Executor::registerComponent(
    Runtime::StoreManager &StoreMgr,
    const Runtime::Instance::ComponentInstance &CompInst) {
  return StoreMgr.registerComponent(&CompInst).map_error([](auto E) {
    E = ErrCode::Value::ModuleNameConflict;
    spdlog::error(E);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
    return E;
  });
}

/// Register a host function which will be invoked before calling a
/// host function.
Expect<void> Executor::registerPreHostFunction(
    void *HostData = nullptr, std::function<void(void *)> HostFunc = nullptr) {
  HostFuncHelper.setPreHost(HostData, HostFunc);
  return {};
}

/// Register a host function which will be invoked after calling a
/// host function.
Expect<void> Executor::registerPostHostFunction(
    void *HostData = nullptr, std::function<void(void *)> HostFunc = nullptr) {
  HostFuncHelper.setPostHost(HostData, HostFunc);
  return {};
}

// Invoke function. See "include/executor/executor.h".
Expect<std::vector<std::pair<ValVariant, ValType>>>
Executor::invoke(const Runtime::Instance::FunctionInstance *FuncInst,
                 Span<const ValVariant> Params,
                 Span<const ValType> ParamTypes) {
  if (unlikely(FuncInst == nullptr)) {
    spdlog::error(ErrCode::Value::FuncNotFound);
    return Unexpect(ErrCode::Value::FuncNotFound);
  }

  // Matching arguments and function type.
  const auto &FuncType = FuncInst->getFuncType();
  const auto &PTypes = FuncType.getParamTypes();
  const auto &RTypes = FuncType.getReturnTypes();
  // The defined type list may be empty if the function is an independent
  // function instance, that is, the module instance will be nullptr. For this
  // case, all of value types are number types or abstract heap types.
  //
  // If a function belongs to component instance, we should totally get
  // converted type, so should no need type list.
  WasmEdge::Span<const WasmEdge::AST::SubType *const> TypeList = {};
  if (FuncInst->getModule()) {
    TypeList = FuncInst->getModule()->getTypeList();
  }
  if (!AST::TypeMatcher::matchTypes(TypeList, ParamTypes, PTypes)) {
    spdlog::error(ErrCode::Value::FuncSigMismatch);
    spdlog::error(ErrInfo::InfoMismatch(
        PTypes, RTypes, std::vector(ParamTypes.begin(), ParamTypes.end()),
        RTypes));
    return Unexpect(ErrCode::Value::FuncSigMismatch);
  }

  // Check the reference value validation.
  for (uint32_t I = 0; I < ParamTypes.size(); ++I) {
    if (ParamTypes[I].isRefType() && (!ParamTypes[I].isNullableRefType() &&
                                      Params[I].get<RefVariant>().isNull())) {
      spdlog::error(ErrCode::Value::NonNullRequired);
      spdlog::error("    Cannot pass a null reference as argument of {}."sv,
                    ParamTypes[I]);
      return Unexpect(ErrCode::Value::NonNullRequired);
    }
  }

  Runtime::StackManager StackMgr;

  // Call runFunction.
  EXPECTED_TRY(runFunction(StackMgr, *FuncInst, Params).map_error([](auto E) {
    dumpStackTrace(Span<const uint32_t>{StackTrace}.first(StackTraceSize));
    return E;
  }));

  // Get return values.
  std::vector<std::pair<ValVariant, ValType>> Returns(RTypes.size());
  for (uint32_t I = 0; I < RTypes.size(); ++I) {
    auto Val = StackMgr.pop();
    const auto &RType = RTypes[RTypes.size() - I - 1];
    if (RType.isRefType()) {
      // For the reference type cases of the return values, they should be
      // transformed into abstract heap types due to the opaque of type indices.
      auto &RefType = Val.get<RefVariant>().getType();
      if (RefType.isExternalized()) {
        // First handle the forced externalized value type case.
        RefType = ValType(TypeCode::Ref, TypeCode::ExternRef);
      }
      if (!RefType.isAbsHeapType()) {
        // The instance must not be nullptr because the null references are
        // already dynamic typed into the top abstract heap type.
        auto *Inst =
            Val.get<RefVariant>().getPtr<Runtime::Instance::CompositeBase>();
        assuming(Inst);
        // The ModInst may be nullptr only in the independent host function
        // instance. Therefore the module instance here must not be nullptr
        // because the independent host function instance cannot be imported and
        // be referred by instructions.
        const auto *ModInst = Inst->getModule();
        auto *DefType = *ModInst->getType(RefType.getTypeIndex());
        RefType =
            ValType(RefType.getCode(), DefType->getCompositeType().expand());
      }
      // Should use the value type from the reference here due to the dynamic
      // typing rule of the null references.
      Returns[RTypes.size() - I - 1] = std::make_pair(Val, RefType);
    } else {
      // For the number type cases of the return values, the unused bits should
      // be erased due to the security issue.
      cleanNumericVal(Val, RType);
      Returns[RTypes.size() - I - 1] = std::make_pair(Val, RType);
    }
  }

  // After execution, the value stack size should be 0.
  assuming(StackMgr.size() == 0);
  return Returns;
}

Async<Expect<std::vector<std::pair<ValVariant, ValType>>>>
Executor::asyncInvoke(const Runtime::Instance::FunctionInstance *FuncInst,
                      Span<const ValVariant> Params,
                      Span<const ValType> ParamTypes) {
  Expect<std::vector<std::pair<ValVariant, ValType>>> (Executor::*FPtr)(
      const Runtime::Instance::FunctionInstance *, Span<const ValVariant>,
      Span<const ValType>) = &Executor::invoke;
  return {FPtr, *this, FuncInst, std::vector(Params.begin(), Params.end()),
          std::vector(ParamTypes.begin(), ParamTypes.end())};
}

// NOTE: due to internal reason, we model the return values can still be
// multiple, but in fact a component function will only return at most one.
// This concept mismatching should be fix in the future.
Expect<std::vector<std::pair<ValInterface, ValType>>>
Executor::invoke(const Runtime::Instance::Component::FunctionInstance *FuncInst,
                 Span<const ValInterface> Params,
                 Span<const ValType> ParamTypes) {
  if (unlikely(FuncInst == nullptr)) {
    spdlog::error(ErrCode::Value::FuncNotFound);
    return Unexpect(ErrCode::Value::FuncNotFound);
  }

  // Matching arguments and function type.
  const auto &FuncType = FuncInst->getFuncType();
  const auto &PTypes = FuncType.getParamTypes();
  const auto &RTypes = FuncType.getReturnTypes();

  Span<const WasmEdge::AST::SubType *const> TypeList = {};
  if (!AST::TypeMatcher::matchTypes(TypeList, ParamTypes, PTypes)) {
    spdlog::error(ErrCode::Value::FuncSigMismatch);
    spdlog::error(ErrInfo::InfoMismatch(
        PTypes, RTypes, std::vector(ParamTypes.begin(), ParamTypes.end()),
        RTypes));
    return Unexpect(ErrCode::Value::FuncSigMismatch);
  }

  auto &HostFunc = FuncInst->getHostFunc();
  std::vector<ValInterface> Rets(RTypes.size());

  EXPECTED_TRY(HostFunc.run(std::move(Params), Rets));

  std::vector<std::pair<ValInterface, ValType>> R;
  auto RType = RTypes.begin();
  for (auto &&V : Rets) {
    R.push_back(std::pair(V, *RType));
    RType++;
  }
  return R;
}

} // namespace Executor
} // namespace WasmEdge
