// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "ast/component/instance.h"
#include "ast/module.h"
#include "common/errcode.h"
#include "common/types.h"
#include "executor/executor.h"

#include "runtime/instance/module.h"

#include <sstream>
#include <string_view>
#include <variant>

namespace WasmEdge {
namespace Executor {

using namespace std::literals;
using namespace AST::Component;
using namespace Runtime;

namespace {
void pushType(Runtime::Instance::ComponentInstance &Comp,
              std::vector<ValType> &Types, const ValueType &VT) {
  // notice that we might need to convert one type to multiple types, and hence,
  // we must let this function control the vector need to be modified.
  auto Func = [&](auto &&Type) {
    using T = std::decay_t<decltype(Type)>;
    if constexpr (std::is_same_v<T, PrimValType>) {
      switch (Type) {
      case PrimValType::Bool:
      case PrimValType::Char:
      case PrimValType::S8:
      case PrimValType::U8:
        Types.push_back(ValType(TypeCode::I8));
        break;
      case PrimValType::S16:
      case PrimValType::U16:
        Types.push_back(ValType(TypeCode::I16));
        break;
      case PrimValType::S32:
      case PrimValType::U32:
        Types.push_back(ValType(TypeCode::I32));
        break;
      case PrimValType::S64:
      case PrimValType::U64:
        Types.push_back(ValType(TypeCode::I64));
        break;
      case PrimValType::F32:
        Types.push_back(ValType(TypeCode::F32));
        break;
      case PrimValType::F64:
        Types.push_back(ValType(TypeCode::F64));
        break;
      case PrimValType::String:
        Types.push_back(InterfaceType(TypeCode::String));
        break;
      case PrimValType::ErrorContext:
        spdlog::warn("Type error-context is not handled yet"sv);
        break;
      }
    } else if constexpr (std::is_same_v<T, uint32_t>) {
      const auto &Ty = Comp.getType(Type);
      spdlog::warn("Type {} is not handled yet"sv, Ty);
    }
  };
  std::visit(Func, VT);
}

AST::FunctionType convert(Runtime::Instance::ComponentInstance &Comp,
                          const FuncType &DT) {
  std::vector<ValType> ParamTypes{};
  for (const auto &P : DT.getParamList()) {
    pushType(Comp, ParamTypes, P.getValType());
  }

  std::vector<ValType> ResultTypes{};
  auto Func = [&](auto &&Type) {
    using T = std::decay_t<decltype(Type)>;
    if constexpr (std::is_same_v<T, ValueType>) {
      pushType(Comp, ResultTypes, Type);
    } else if constexpr (std::is_same_v<T, std::vector<LabelValType>>) {
      for (const auto &R : Type) {
        pushType(Comp, ResultTypes, R.getValType());
      }
    }
  };
  std::visit(Func, DT.getResultList());

  return AST::FunctionType(ParamTypes, ResultTypes);
}
} // namespace

class LiftTrans : public WasmEdge::Runtime::Component::HostFunctionBase {
public:
  LiftTrans(Executor *Exec, const AST::Component::FuncType &DefinedType,
            Instance::FunctionInstance *Func, Instance::MemoryInstance *M,
            Instance::FunctionInstance *R,
            Runtime::Instance::ComponentInstance &Comp)
      : HostFunctionBase(), Exec(Exec), LowerFunc(Func), Memory(M), Realloc(R) {
    // The convert is simply let component type to internal type.
    FuncType = convert(Comp, DefinedType);
    spdlog::info("lifted: {}"sv, FuncType);
  }

  Expect<void> run(Span<const ValInterface> Args,
                   Span<ValInterface> Rets) override {
    const auto &HigherFuncType = FuncType;

    uint32_t PI = 0;
    std::vector<ValVariant> LowerArgs{};
    for (auto &ParamTy : HigherFuncType.getParamTypes()) {
      switch (ParamTy.getCode()) {
      case TypeCode::String: {
        std::string_view Str = std::get<std::string>(Args[PI++]);

        auto StrSize = static_cast<uint32_t>(Str.size());
        std::vector<ValVariant> ReallocArgs{ValVariant(0), ValVariant(0),
                                            ValVariant(0), ValVariant(StrSize)};
        EXPECTED_TRY(auto RPtr,
                     Exec->invoke(Realloc, ReallocArgs,
                                  Realloc->getFuncType().getParamTypes()));
        ValVariant PtrInMem = RPtr[0].first;

        Memory->setBytes(std::vector<Byte>{Str.begin(), Str.end()},
                         PtrInMem.get<uint32_t>(), 0,
                         static_cast<uint32_t>(Str.size()));
        LowerArgs.push_back(PtrInMem);
        LowerArgs.push_back(StrSize);
        break;
      }
      default: {
        // usual type has no need conversion
        const ValVariant &Arg = std::get<ValVariant>(Args[PI++]);
        LowerArgs.push_back(Arg);
        break;
      }
      }
    }

    auto &LowerFuncType = LowerFunc->getFuncType();
    EXPECTED_TRY(auto ResultList, Exec->invoke(LowerFunc, LowerArgs,
                                               LowerFuncType.getParamTypes()));
    uint32_t RI = 0;
    uint32_t TakeI = 0;
    for (auto const &HighTy : HigherFuncType.getReturnTypes()) {
      switch (HighTy.getCode()) {
      case TypeCode::String: {
        auto Idx = ResultList[TakeI++].first.get<uint32_t>();
        auto Size = ResultList[TakeI++].first.get<uint32_t>();
        auto Str = Memory->getStringView(Idx, Size);
        Rets[RI++].emplace<std::string>(std::string(Str.begin(), Str.end()));
        break;
      }
      default: {
        Rets[RI++].emplace<ValVariant>(ResultList[TakeI++].first);
        break;
      }
      }
    }

    return {};
  }

private:
  Executor *Exec;
  Instance::FunctionInstance *LowerFunc;
  Instance::MemoryInstance *Memory;
  Instance::FunctionInstance *Realloc;
};

std::unique_ptr<Instance::Component::FunctionInstance>
Executor::lifting(Runtime::Instance::ComponentInstance &Comp,
                  const FuncType &FuncType, Instance::FunctionInstance *Func,
                  Instance::MemoryInstance *Memory,
                  Instance::FunctionInstance *Realloc) {
  auto R = std::make_unique<Instance::Component::FunctionInstance>(
      std::make_unique<LiftTrans>(this, FuncType, Func, Memory, Realloc, Comp));
  return R;
}

class LowerTrans : public HostFunctionBase {
public:
  LowerTrans(Executor *Exec, Instance::Component::FunctionInstance *Func,
             Instance::MemoryInstance *Memory,
             Instance::FunctionInstance *Realloc)
      : HostFunctionBase(0), Exec(Exec), HigherFunc(Func), Memory(Memory),
        Realloc(Realloc) {
    auto &HigherType = HigherFunc->getFuncType();

    auto &FuncType = DefType.getCompositeType().getFuncType();
    for (auto &ParamTy : HigherType.getParamTypes()) {
      switch (ParamTy.getCode()) {
      case TypeCode::String:
        FuncType.getParamTypes().push_back(TypeCode::I32);
        FuncType.getParamTypes().push_back(TypeCode::I32);
        break;
      default:
        FuncType.getParamTypes().push_back(ParamTy);
        break;
      }
    }

    for (auto &ReturnTy : HigherType.getReturnTypes()) {
      switch (ReturnTy.getCode()) {
      case TypeCode::String:
        FuncType.getReturnTypes().push_back(TypeCode::I32);
        FuncType.getReturnTypes().push_back(TypeCode::I32);
        break;
      default:
        FuncType.getReturnTypes().push_back(ReturnTy);
        break;
      }
    }

    spdlog::info("lower: {}"sv, FuncType);
  }

  Expect<void> run(const Runtime::CallingFrame &, Span<const ValVariant> Args,
                   Span<ValVariant> Rets) override {
    auto &HigherFuncType = HigherFunc->getFuncType();

    uint32_t PI = 0;
    std::vector<ValInterface> HigherArgs{};
    for (auto &ParamTy : HigherFuncType.getParamTypes()) {
      switch (ParamTy.getCode()) {
      case TypeCode::String: {
        auto Idx = Args[PI++];
        auto Len = Args[PI++];
        std::string_view V =
            Memory->getStringView(Idx.get<uint32_t>(), Len.get<uint32_t>());
        ValInterface VI{};
        VI.emplace<std::string>(std::string(V.begin(), V.end()));
        HigherArgs.push_back(VI);
        break;
      }
      default:
        auto Arg = Args[PI++];
        HigherArgs.push_back(Arg);
        break;
      }
    }

    EXPECTED_TRY(auto ResultList, Exec->invoke(HigherFunc, HigherArgs,
                                               HigherFuncType.getParamTypes()));
    uint32_t RI = 0;
    for (auto &[RetVal, RetTy] : ResultList) {
      switch (RetTy.getCode()) {
      case TypeCode::String: {
        auto const &Str = std::get<std::string>(RetVal);

        auto StrSize = static_cast<uint32_t>(Str.size());
        std::vector<ValVariant> ReallocArgs{ValVariant(0), ValVariant(0),
                                            ValVariant(0), ValVariant(StrSize)};
        EXPECTED_TRY(auto RPtr,
                     Exec->invoke(Realloc, ReallocArgs,
                                  Realloc->getFuncType().getParamTypes()));
        ValVariant V = RPtr[0].first;

        Memory->setBytes(std::vector<Byte>{Str.begin(), Str.end()},
                         V.get<uint32_t>(), 0,
                         static_cast<uint32_t>(Str.size()));
        Rets[RI++] = V;
        Rets[RI++] = ValVariant(StrSize);
        break;
      }
      default:
        Rets[RI++] = std::get<ValVariant>(RetVal);
        break;
      }
    }

    return {};
  }

private:
  Executor *Exec;
  /* HigherFunc: a component function we are wrapping
   */
  Instance::Component::FunctionInstance *HigherFunc;
  /* Memory: the shared memory from a certain core module defined in component
   */
  Instance::MemoryInstance *Memory;
  Instance::FunctionInstance *Realloc;
};

std::unique_ptr<Instance::FunctionInstance>
Executor::lowering(Instance::Component::FunctionInstance *Func,
                   Instance::MemoryInstance *Memory,
                   Instance::FunctionInstance *Realloc) {
  auto R = std::make_unique<Instance::FunctionInstance>(
      std::make_unique<LowerTrans>(this, Func, Memory, Realloc));
  return R;
}

Expect<void>
Executor::instantiate(Runtime::StoreManager &,
                      Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::CanonSection &CanonSec) {
  for (auto &Content : CanonSec.getContent()) {
    auto Func1 = [this, &CompInst](auto &&C) -> Expect<void> {
      using T = std::decay_t<decltype(C)>;
      if constexpr (std::is_same_v<T, Lift>) {
        // lift wrap a core wasm function to a component function, with proper
        // modification about canonical ABI.
        const auto &Opts = C.getOptions();
        Runtime::Instance::MemoryInstance *Mem = nullptr;
        Runtime::Instance::FunctionInstance *ReallocFunc = nullptr;
        for (auto &Opt : Opts) {
          auto Func2 = [&](auto &&O) -> Expect<void> {
            using U = std::decay_t<decltype(O)>;
            if constexpr (std::is_same_v<U, StringEncoding>) {
              spdlog::warn("incomplete canonical option `string-encoding`"sv);
            } else if constexpr (std::is_same_v<U, Memory>) {
              auto MemIdx = O.getMemIndex();
              Mem = CompInst.getCoreMemoryInstance(MemIdx);
            } else if constexpr (std::is_same_v<U, Realloc>) {
              ReallocFunc = CompInst.getCoreFunctionInstance(O.getFuncIndex());
            } else if constexpr (std::is_same_v<U, PostReturn>) {
              spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Canon));
              return Unexpect(ErrCode::Value::InvalidCanonOption);
            }
            return {};
          };
          EXPECTED_TRY(std::visit(Func2, Opt));
        }

        const auto &AstFuncType = CompInst.getType(C.getFuncTypeIndex());
        if (unlikely(!std::holds_alternative<FuncType>(AstFuncType))) {
          // It doesn't make sense if one tries to lift an instance not a
          // function, so unlikely happen.
          spdlog::error("cannot lift a non-function"sv);
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Canon));
          return Unexpect(ErrCode::Value::InvalidCanonOption);
        }

        auto *FuncInst = CompInst.getCoreFunctionInstance(C.getCoreFuncIndex());
        CompInst.addFunctionInstance(lifting(CompInst,
                                             std::get<FuncType>(AstFuncType),
                                             FuncInst, Mem, ReallocFunc));
      } else if constexpr (std::is_same_v<T, Lower>) {
        // lower sends a component function to a core wasm function, with proper
        // modification about canonical ABI.
        const auto &Opts = C.getOptions();
        Runtime::Instance::MemoryInstance *Mem = nullptr;
        Runtime::Instance::FunctionInstance *ReallocFunc = nullptr;
        for (auto &Opt : Opts) {
          auto Func2 = [&](auto &&O) -> Expect<void> {
            using U = std::decay_t<decltype(O)>;
            if constexpr (std::is_same_v<U, StringEncoding>) {
              spdlog::warn("incomplete canonical option `string-encoding`"sv);
              return Unexpect(ErrCode::Value::InvalidCanonOption);
            } else if constexpr (std::is_same_v<U, Memory>) {
              auto MemIdx = O.getMemIndex();
              Mem = CompInst.getCoreMemoryInstance(MemIdx);
            } else if constexpr (std::is_same_v<U, Realloc>) {
              ReallocFunc = CompInst.getCoreFunctionInstance(O.getFuncIndex());
            } else if constexpr (std::is_same_v<U, PostReturn>) {
              spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Canon));
              return Unexpect(ErrCode::Value::InvalidCanonOption);
            }
            return {};
          };
          EXPECTED_TRY(std::visit(Func2, Opt));
        }

        auto *FuncInst = CompInst.getFunctionInstance(C.getFuncIndex());
        CompInst.addCoreFunctionInstance(lowering(FuncInst, Mem, ReallocFunc));
      } else if constexpr (std::is_same_v<T, ResourceNew>) {
        spdlog::warn("resource is not supported yet"sv);
        return Unexpect(ErrCode::Value::InvalidCanonOption);
      } else if constexpr (std::is_same_v<T, ResourceDrop>) {
        spdlog::warn("resource is not supported yet"sv);
        return Unexpect(ErrCode::Value::InvalidCanonOption);
      } else if constexpr (std::is_same_v<T, ResourceRep>) {
        spdlog::warn("resource is not supported yet"sv);
        return Unexpect(ErrCode::Value::InvalidCanonOption);
      }
      return {};
    };
    EXPECTED_TRY(std::visit(Func1, Content));
  }

  return {};
}

} // namespace Executor
} // namespace WasmEdge
