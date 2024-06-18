#include "ast/component/instance.h"
#include "ast/module.h"
#include "common/errcode.h"
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
              std::vector<ValType> &Types, const ValueType &T) {
  // notice that we might need to convert one type to multiple types, and hence,
  // we must let this function control the vector need to be modified.
  if (std::holds_alternative<PrimValType>(T)) {
    switch (std::get<PrimValType>(T)) {
    case PrimValType::Bool:
    case PrimValType::Char:
    case PrimValType::S8:
    case PrimValType::U8:
      Types.push_back(TypeCode::I8);
      break;
    case PrimValType::S16:
    case PrimValType::U16:
      Types.push_back(TypeCode::I16);
      break;
    case PrimValType::S32:
    case PrimValType::U32:
      Types.push_back(TypeCode::I32);
      break;
    case PrimValType::S64:
    case PrimValType::U64:
      Types.push_back(TypeCode::I64);
      break;
    case PrimValType::Float32:
      Types.push_back(TypeCode::F32);
      break;
    case PrimValType::Float64:
      Types.push_back(TypeCode::F64);
      break;
    case PrimValType::String:
      Types.push_back(TypeCode::String);
      break;
    }
  } else {
    auto Idx = std::get<TypeIndex>(T);
    const auto &Ty = Comp.getType(Idx);
    spdlog::warn("Type {} is not handled yet"sv, Ty);
  }
}

AST::FunctionType convert(Runtime::Instance::ComponentInstance &Comp,
                          const FuncType &DT) {
  std::vector<ValType> ParamTypes{};
  for (const auto &P : DT.getParamList()) {
    pushType(Comp, ParamTypes, P.getValType());
  }

  std::vector<ValType> ResultTypes{};
  if (std::holds_alternative<ValueType>(DT.getResultList())) {
    pushType(Comp, ResultTypes, std::get<ValueType>(DT.getResultList()));
  } else {
    const auto &RL = DT.getResultList();
    for (const auto &R : std::get<std::vector<LabelValType>>(RL)) {
      pushType(Comp, ResultTypes, R.getValType());
    }
  }

  return AST::FunctionType(ParamTypes, ResultTypes);
}
} // namespace

class LiftTrans : public HostFunctionBase {
public:
  LiftTrans(Executor *Exec, const FuncType &DefinedType,
            Instance::FunctionInstance *Func, Instance::MemoryInstance *M,
            Instance::FunctionInstance *R,
            Runtime::Instance::ComponentInstance &Comp)
      : HostFunctionBase(0), Exec(Exec), LowerFunc(Func), Memory(M),
        Realloc(R) {
    auto &FT = DefType.getCompositeType().getFuncType();
    // The convert is simply let component type to internal type.
    FT = convert(Comp, DefinedType);
    spdlog::info("lifted: {}"sv, FT);
  }

  Expect<void> run(const Runtime::CallingFrame &, Span<const ValVariant> Args,
                   Span<ValVariant> Rets) override {
    const auto &HigherFuncType = DefType.getCompositeType().getFuncType();

    uint32_t PI = 0;
    std::vector<ValVariant> LowerArgs{};
    for (auto &ParamTy : HigherFuncType.getParamTypes()) {
      switch (ParamTy.getCode()) {
      case TypeCode::String: {
        std::string_view Str = Args[PI++].get<StrVariant>().getString();

        auto StrSize = static_cast<uint32_t>(Str.size());
        std::vector<ValVariant> ReallocArgs{ValVariant(0), ValVariant(0),
                                            ValVariant(0), ValVariant(StrSize)};
        auto RPtr = Exec->invoke(Realloc, ReallocArgs,
                                 Realloc->getFuncType().getParamTypes());
        if (!RPtr) {
          return Unexpect(RPtr);
        }
        ValVariant PtrInMem = (*RPtr)[0].first;

        Memory->setBytes(std::vector<Byte>{Str.begin(), Str.end()},
                         PtrInMem.get<uint32_t>(), 0,
                         static_cast<uint32_t>(Str.size()));
        LowerArgs.push_back(PtrInMem);
        LowerArgs.push_back(StrSize);
        break;
      }
      default: {
        // usual type has no need conversion
        const ValVariant &Arg = Args[PI++];
        LowerArgs.push_back(Arg);
        break;
      }
      }
    }

    auto &LowerFuncType = LowerFunc->getFuncType();
    auto Res =
        Exec->invoke(LowerFunc, LowerArgs, LowerFuncType.getParamTypes());
    if (!Res) {
      return Unexpect(Res);
    }

    uint32_t RI = 0;
    uint32_t TakeI = 0;
    auto &ResultList = *Res;
    for (auto &HighTy : HigherFuncType.getReturnTypes()) {
      switch (HighTy.getCode()) {
      case TypeCode::String: {
        auto Idx = ResultList[TakeI++].first.get<uint32_t>();
        auto Size = ResultList[TakeI++].first.get<uint32_t>();
        auto Str = Memory->getStringView(Idx, Size);
        Rets[RI++] = StrVariant(std::string(Str.begin(), Str.end()));
        break;
      }
      default: {
        Rets[RI++] = ResultList[TakeI++].first;
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

std::unique_ptr<Instance::FunctionInstance>
Executor::lifting(Runtime::Instance::ComponentInstance &Comp,
                  const FuncType &FuncType, Instance::FunctionInstance *Func,
                  Instance::MemoryInstance *Memory,
                  Instance::FunctionInstance *Realloc) {
  auto R = std::make_unique<Instance::FunctionInstance>(
      std::make_unique<LiftTrans>(this, FuncType, Func, Memory, Realloc, Comp));
  return R;
}

class LowerTrans : public HostFunctionBase {
public:
  LowerTrans(Executor *Exec, Instance::FunctionInstance *Func,
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
    std::vector<ValVariant> HigherArgs{};
    for (auto &ParamTy : HigherFuncType.getParamTypes()) {
      switch (ParamTy.getCode()) {
      case TypeCode::String: {
        auto Idx = Args[PI++];
        auto Len = Args[PI++];
        std::string_view V =
            Memory->getStringView(Idx.get<uint32_t>(), Len.get<uint32_t>());
        std::string S{V.begin(), V.end()};
        HigherArgs.push_back(StrVariant(std::move(S)));
        break;
      }
      default:
        // usual type has no need conversion
        const ValVariant &Arg = Args[PI++];
        HigherArgs.push_back(Arg);
        break;
      }
    }

    auto Res =
        Exec->invoke(HigherFunc, HigherArgs, HigherFuncType.getParamTypes());
    if (!Res) {
      return Unexpect(Res);
    }

    uint32_t RI = 0;
    for (auto &[RetVal, RetTy] : *Res) {
      switch (RetTy.getCode()) {
      case TypeCode::String: {
        auto const &Str = RetVal.get<StrVariant>().getString();

        auto StrSize = static_cast<uint32_t>(Str.size());
        std::vector<ValVariant> ReallocArgs{ValVariant(0), ValVariant(0),
                                            ValVariant(0), ValVariant(StrSize)};
        auto RPtr = Exec->invoke(Realloc, ReallocArgs,
                                 Realloc->getFuncType().getParamTypes());
        if (!RPtr) {
          return Unexpect(RPtr);
        }
        ValVariant V = (*RPtr)[0].first;

        Memory->setBytes(std::vector<Byte>{Str.begin(), Str.end()},
                         V.get<uint32_t>(), 0,
                         static_cast<uint32_t>(Str.size()));
        Rets[RI++] = V;
        Rets[RI++] = ValVariant(StrSize);
        break;
      }
      default:
        Rets[RI++] = RetVal;
        break;
      }
    }

    return {};
  }

private:
  Executor *Exec;
  Instance::FunctionInstance *HigherFunc;
  Instance::MemoryInstance *Memory;
  Instance::FunctionInstance *Realloc;
};

std::unique_ptr<Instance::FunctionInstance>
Executor::lowering(Instance::FunctionInstance *Func,
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
  for (auto &C : CanonSec.getContent()) {
    if (std::holds_alternative<Lift>(C)) {
      // lift wrap a core wasm function to a component function, with proper
      // modification about canonical ABI.

      const auto &L = std::get<Lift>(C);

      const auto &Opts = L.getOptions();

      Runtime::Instance::MemoryInstance *Mem = nullptr;
      Runtime::Instance::FunctionInstance *ReallocFunc = nullptr;
      for (auto &Opt : Opts) {
        if (std::holds_alternative<StringEncoding>(Opt)) {
          spdlog::warn("incomplete canonical option `string-encoding`"sv);
        } else if (std::holds_alternative<Memory>(Opt)) {
          auto MemIdx = std::get<Memory>(Opt).getMemIndex();
          Mem = CompInst.getCoreMemoryInstance(MemIdx);
        } else if (std::holds_alternative<Realloc>(Opt)) {
          ReallocFunc = CompInst.getCoreFunctionInstance(
              std::get<Realloc>(Opt).getFuncIndex());
        } else if (std::holds_alternative<PostReturn>(Opt)) {
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Canon));
          return Unexpect(ErrCode::Value::InvalidCanonOption);
        }
      }

      const auto &AstFuncType = CompInst.getType(L.getFuncTypeIndex());
      if (unlikely(!std::holds_alternative<FuncType>(AstFuncType))) {
        // It doesn't make sense if one tries to lift an instance not a
        // function, so unlikely happen.
        spdlog::error("cannot lift a non-function"sv);
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Canon));
        return Unexpect(ErrCode::Value::InvalidCanonOption);
      }

      auto *FuncInst = CompInst.getCoreFunctionInstance(L.getCoreFuncIndex());
      CompInst.addFunctionInstance(lifting(CompInst,
                                           std::get<FuncType>(AstFuncType),
                                           FuncInst, Mem, ReallocFunc));
    } else if (std::holds_alternative<Lower>(C)) {
      // lower sends a component function to a core wasm function, with proper
      // modification about canonical ABI.
      const auto &L = std::get<Lower>(C);

      Runtime::Instance::MemoryInstance *Mem = nullptr;
      Runtime::Instance::FunctionInstance *ReallocFunc = nullptr;
      const auto &Opts = L.getOptions();
      for (auto &Opt : Opts) {
        if (std::holds_alternative<StringEncoding>(Opt)) {
          spdlog::warn("incomplete canonical option `string-encoding`"sv);
          return Unexpect(ErrCode::Value::InvalidCanonOption);
        } else if (std::holds_alternative<Memory>(Opt)) {
          auto MemIdx = std::get<Memory>(Opt).getMemIndex();
          Mem = CompInst.getCoreMemoryInstance(MemIdx);
        } else if (std::holds_alternative<Realloc>(Opt)) {
          ReallocFunc = CompInst.getCoreFunctionInstance(
              std::get<Realloc>(Opt).getFuncIndex());
        } else if (std::holds_alternative<PostReturn>(Opt)) {
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Canon));
          return Unexpect(ErrCode::Value::InvalidCanonOption);
        }
      }

      auto *FuncInst = CompInst.getFunctionInstance(L.getFuncIndex());
      CompInst.addCoreFunctionInstance(lowering(FuncInst, Mem, ReallocFunc));
    } else if (std::holds_alternative<ResourceNew>(C)) {
      spdlog::warn("resource is not supported yet"sv);
      return Unexpect(ErrCode::Value::InvalidCanonOption);
    } else if (std::holds_alternative<ResourceDrop>(C)) {
      spdlog::warn("resource is not supported yet"sv);
      return Unexpect(ErrCode::Value::InvalidCanonOption);
    } else if (std::holds_alternative<ResourceRep>(C)) {
      spdlog::warn("resource is not supported yet"sv);
      return Unexpect(ErrCode::Value::InvalidCanonOption);
    }
  }

  return {};
}

} // namespace Executor
} // namespace WasmEdge
