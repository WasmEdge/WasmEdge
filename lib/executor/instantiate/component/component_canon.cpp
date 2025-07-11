// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <string_view>

namespace WasmEdge {
namespace Executor {

using namespace std::literals;

namespace {
void pushType(Runtime::Instance::ComponentInstance &,
              std::vector<ValType> &Types,
              const AST::Component::ValueType &VT) {
  // Noticed that we might need to convert one type into multiple types, hence
  // we must keep the types vector being mutable.
  if (VT.isPrimValType()) {
    switch (VT.getCode()) {
    case AST::Component::PrimValType::Bool:
    case AST::Component::PrimValType::Char:
    case AST::Component::PrimValType::S8:
    case AST::Component::PrimValType::U8:
      Types.push_back(ValType(TypeCode::I8));
      break;
    case AST::Component::PrimValType::S16:
    case AST::Component::PrimValType::U16:
      Types.push_back(ValType(TypeCode::I16));
      break;
    case AST::Component::PrimValType::S32:
    case AST::Component::PrimValType::U32:
      Types.push_back(ValType(TypeCode::I32));
      break;
    case AST::Component::PrimValType::S64:
    case AST::Component::PrimValType::U64:
      Types.push_back(ValType(TypeCode::I64));
      break;
    case AST::Component::PrimValType::F32:
      Types.push_back(ValType(TypeCode::F32));
      break;
    case AST::Component::PrimValType::F64:
      Types.push_back(ValType(TypeCode::F64));
      break;
    case AST::Component::PrimValType::String:
      Types.push_back(InterfaceType(TypeCode::String));
      break;
    case AST::Component::PrimValType::ErrorContext:
      spdlog::error("Type error-context is not handled yet"sv);
      break;
    default:
      assumingUnreachable();
    }
  } else {
    spdlog::error("Type index {} case is not handled yet"sv, VT.getTypeIndex());
  }
}
} // namespace

class LiftTrans : public Runtime::Instance::Component::HostFunctionBase {
public:
  LiftTrans(Executor *E, const AST::Component::FuncType &DefinedType,
            Runtime::Instance::FunctionInstance *F,
            Runtime::Instance::MemoryInstance *M,
            Runtime::Instance::FunctionInstance *R,
            Runtime::Instance::ComponentInstance &Comp)
      : Runtime::Instance::Component::HostFunctionBase(), Exec(E), LowerFunc(F),
        Memory(M), Realloc(R) {
    for (const auto &PType : DefinedType.getParamList()) {
      pushType(Comp, FuncType.getParamTypes(), PType.getValType());
    }

    if (DefinedType.getResultArity() == 1) {
      pushType(Comp, FuncType.getReturnTypes(), DefinedType.getResultType());
    } else {
      for (const auto &RType : DefinedType.getResultList()) {
        pushType(Comp, FuncType.getReturnTypes(), RType.getValType());
      }
    }
  }

  Expect<void> run(Span<const ValInterface> Args,
                   Span<ValInterface> Rets) override {
    const auto &HigherFuncType = FuncType;

    uint32_t PI = 0;
    std::vector<ValVariant> LowerArgs;
    for (const auto &PType : HigherFuncType.getParamTypes()) {
      switch (PType.getCode()) {
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
        // Other types don't need conversion
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
  Runtime::Instance::FunctionInstance *LowerFunc;
  Runtime::Instance::MemoryInstance *Memory;
  Runtime::Instance::FunctionInstance *Realloc;
};

std::unique_ptr<Runtime::Instance::Component::FunctionInstance>
Executor::lifting(Runtime::Instance::ComponentInstance &Comp,
                  const AST::Component::FuncType &FuncType,
                  Runtime::Instance::FunctionInstance *Func,
                  Runtime::Instance::MemoryInstance *Memory,
                  Runtime::Instance::FunctionInstance *Realloc) {
  auto R = std::make_unique<Runtime::Instance::Component::FunctionInstance>(
      std::make_unique<LiftTrans>(this, FuncType, Func, Memory, Realloc, Comp));
  return R;
}

class LowerTrans : public Runtime::HostFunctionBase {
public:
  LowerTrans(Executor *E, Runtime::Instance::Component::FunctionInstance *F,
             Runtime::Instance::MemoryInstance *M,
             Runtime::Instance::FunctionInstance *R)
      : Runtime::HostFunctionBase(0), Exec(E), HigherFunc(F), Memory(M),
        Realloc(R) {
    const auto &HigherType = HigherFunc->getFuncType();

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
  }

  Expect<void> run(const Runtime::CallingFrame &, Span<const ValVariant> Args,
                   Span<ValVariant> Rets) override {
    auto &HigherFuncType = HigherFunc->getFuncType();

    uint32_t PI = 0;
    std::vector<ValInterface> HigherArgs;
    for (auto &ParamTy : HigherFuncType.getParamTypes()) {
      switch (ParamTy.getCode()) {
      case TypeCode::String: {
        auto Idx = Args[PI++];
        auto Len = Args[PI++];
        std::string_view V =
            Memory->getStringView(Idx.get<uint32_t>(), Len.get<uint32_t>());
        ValInterface VI;
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
  // The component function to wrap
  Runtime::Instance::Component::FunctionInstance *HigherFunc;
  // The shared memory from the certain core module defined in component
  Runtime::Instance::MemoryInstance *Memory;
  Runtime::Instance::FunctionInstance *Realloc;
};

std::unique_ptr<Runtime::Instance::FunctionInstance>
Executor::lowering(Runtime::Instance::Component::FunctionInstance *Func,
                   Runtime::Instance::MemoryInstance *Memory,
                   Runtime::Instance::FunctionInstance *Realloc) {
  auto R = std::make_unique<Runtime::Instance::FunctionInstance>(
      std::make_unique<LowerTrans>(this, Func, Memory, Realloc));
  return R;
}

Expect<void>
Executor::instantiate(Runtime::StoreManager &,
                      Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::CanonSection &CanonSec) {
  for (const auto &Canon : CanonSec.getContent()) {
    switch (Canon.getOpCode()) {
    case AST::Component::Canonical::OpCode::Lift: {
      // lift wrap a core wasm function to a component function, with proper
      // modification about canonical ABI.
      const auto &Opts = Canon.getOptions();
      Runtime::Instance::MemoryInstance *MemInst = nullptr;
      Runtime::Instance::FunctionInstance *ReallocFunc = nullptr;
      for (auto &Opt : Opts) {
        switch (Opt.getCode()) {
        case AST::Component::CanonOpt::OptCode::Encode_UTF8:
        case AST::Component::CanonOpt::OptCode::Encode_UTF16:
        case AST::Component::CanonOpt::OptCode::Encode_Latin1:
          spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
          spdlog::error("    incomplete canonincal options"sv);
          return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
        case AST::Component::CanonOpt::OptCode::Memory:
          MemInst = CompInst.getCoreMemoryInstance(Opt.getIndex());
          break;
        case AST::Component::CanonOpt::OptCode::Realloc:
          ReallocFunc = CompInst.getCoreFunctionInstance(Opt.getIndex());
          break;
        case AST::Component::CanonOpt::OptCode::PostReturn:
        case AST::Component::CanonOpt::OptCode::Async:
          // TODO: incomplete validation of these cases.
        default:
          assumingUnreachable();
        }
      }

      const auto &DType = CompInst.getType(Canon.getTargetIndex());
      if (unlikely(!DType.isFuncType())) {
        // It doesn't make sense if one tries to lift an instance not a
        // function, so unlikely happen.
        spdlog::error(ErrCode::Value::InvalidCanonOption);
        spdlog::error("    Cannot lift a non-function"sv);
        return Unexpect(ErrCode::Value::InvalidCanonOption);
      }
      auto *FuncInst = CompInst.getCoreFunctionInstance(Canon.getIndex());
      CompInst.addFunctionInstance(lifting(CompInst, DType.getFuncType(),
                                           FuncInst, MemInst, ReallocFunc));
      break;
    }
    case AST::Component::Canonical::OpCode::Lower: {
      // lower sends a component function to a core wasm function, with proper
      // modification about canonical ABI.
      const auto &Opts = Canon.getOptions();
      Runtime::Instance::MemoryInstance *MemInst = nullptr;
      Runtime::Instance::FunctionInstance *ReallocFunc = nullptr;
      for (auto &Opt : Opts) {
        switch (Opt.getCode()) {
        case AST::Component::CanonOpt::OptCode::Encode_UTF8:
        case AST::Component::CanonOpt::OptCode::Encode_UTF16:
        case AST::Component::CanonOpt::OptCode::Encode_Latin1:
          spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
          spdlog::error("    incomplete canonincal options"sv);
          return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
        case AST::Component::CanonOpt::OptCode::Memory:
          MemInst = CompInst.getCoreMemoryInstance(Opt.getIndex());
          break;
        case AST::Component::CanonOpt::OptCode::Realloc:
          ReallocFunc = CompInst.getCoreFunctionInstance(Opt.getIndex());
          break;
        case AST::Component::CanonOpt::OptCode::PostReturn:
        case AST::Component::CanonOpt::OptCode::Async:
          // TODO: incomplete validation of these cases.
        default:
          assumingUnreachable();
        }
      }

      auto *FuncInst = CompInst.getFunctionInstance(Canon.getIndex());
      CompInst.addCoreFunctionInstance(
          lowering(FuncInst, MemInst, ReallocFunc));
      break;
    }
    case AST::Component::Canonical::OpCode::Resource__new:
    case AST::Component::Canonical::OpCode::Resource__drop:
    case AST::Component::Canonical::OpCode::Resource__rep:
    default:
      spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
      spdlog::error("    incomplete canonincal"sv);
      return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
    }
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
