#include "ast/component/instance.h"
#include "ast/module.h"
#include "common/errcode.h"
#include "common/types.h"
#include "executor/component/canon.h"
#include "executor/executor.h"
#include "runtime/instance/module.h"
#include "spdlog/spdlog.h"

#include <cmath>
#include <sstream>
#include <string_view>
#include <variant>

namespace WasmEdge {
namespace Executor {

using namespace std::literals;
using namespace AST::Component;
using namespace Runtime;

namespace {

Expect<void> pushDefType(Runtime::Instance::ComponentInstance &Comp,
                         std::vector<InterfaceType> &Types, const DefType &DT);

Expect<void> pushPrimValType(Runtime::Instance::ComponentInstance &,
                             std::vector<InterfaceType> &Types,
                             const PrimValType &PT) {
  switch (PT) {
  case PrimValType::Bool:
    Types.push_back(InterfaceType(TypeCode::Bool));
    break;
  case PrimValType::S8:
    Types.push_back(InterfaceType(TypeCode::I8));
    break;
  case PrimValType::U8:
    Types.push_back(InterfaceType(TypeCode::U8));
    break;
  case PrimValType::S16:
    Types.push_back(InterfaceType(TypeCode::I16));
    break;
  case PrimValType::U16:
    Types.push_back(InterfaceType(TypeCode::U16));
    break;
  case PrimValType::S32:
    Types.push_back(InterfaceType(TypeCode::I32));
    break;
  case PrimValType::U32:
    Types.push_back(InterfaceType(TypeCode::U32));
    break;
  case PrimValType::S64:
    Types.push_back(InterfaceType(TypeCode::I64));
    break;
  case PrimValType::U64:
    Types.push_back(InterfaceType(TypeCode::U64));
    break;
  case PrimValType::Float32:
    Types.push_back(InterfaceType(TypeCode::F32));
    break;
  case PrimValType::Float64:
    Types.push_back(InterfaceType(TypeCode::F64));
    break;
  case PrimValType::Char:
    Types.push_back(InterfaceType(TypeCode::U32));
    break;
  case PrimValType::String:
    Types.push_back(InterfaceType(TypeCode::String));
    break;
  default:
    assumingUnreachable();
  }
  return {};
}

Expect<void> pushDefValType(Runtime::Instance::ComponentInstance &Comp,
                            std::vector<InterfaceType> &Types,
                            const DefValType &DT) {
  if (std::holds_alternative<PrimValType>(DT)) {
    pushPrimValType(Comp, Types, std::get<PrimValType>(DT));
  } else if (std::holds_alternative<RecordTy>(DT)) {
    spdlog::warn("record type is not handled yet"sv, DT);
  } else if (std::holds_alternative<VariantTy>(DT)) {
    spdlog::warn("variant type is not handled yet"sv, DT);
  } else if (std::holds_alternative<ListTy>(DT)) {
    spdlog::warn("list type is not handled yet"sv, DT);
  } else if (std::holds_alternative<TupleTy>(DT)) {
    spdlog::warn("tuple type is not handled yet"sv, DT);
  } else if (std::holds_alternative<Flags>(DT)) {
    spdlog::warn("flags type is not handled yet"sv, DT);
  } else if (std::holds_alternative<EnumTy>(DT)) {
    spdlog::warn("enum type is not handled yet"sv, DT);
  } else if (std::holds_alternative<OptionTy>(DT)) {
    spdlog::warn("option type is not handled yet"sv, DT);
  } else if (std::holds_alternative<ResultTy>(DT)) {
    spdlog::warn("result type is not handled yet"sv, DT);
  } else if (std::holds_alternative<Own>(DT)) {
    auto O = std::get<Own>(DT);
    EXPECTED_TRY(auto T, Comp.getType(O.getIndex()));
    std::vector<InterfaceType> TyArgs{};
    pushDefType(Comp, TyArgs, T);
    Types.push_back(InterfaceType(TypeCode::Own, TyArgs));
  } else if (std::holds_alternative<Borrow>(DT)) {
    spdlog::warn("borrow type is not handled yet"sv, DT);
  } else {
    assumingUnreachable();
  }

  return {};
}

Expect<void> pushDefType(Runtime::Instance::ComponentInstance &Comp,
                         std::vector<InterfaceType> &Types, const DefType &DT) {
  if (std::holds_alternative<DefValType>(DT)) {
    pushDefValType(Comp, Types, std::get<DefValType>(DT));
  } else if (std::holds_alternative<FuncType>(DT)) {
    spdlog::warn("function type is not handled yet"sv, DT);
  } else if (std::holds_alternative<ComponentType>(DT)) {
    spdlog::warn("component type is not handled yet"sv, DT);
  } else if (std::holds_alternative<InstanceType>(DT)) {
    spdlog::warn("instance type is not handled yet"sv, DT);
  } else if (std::holds_alternative<ResourceType>(DT)) {
    spdlog::warn("resource type is not handled yet"sv, DT);
  } else {
    assumingUnreachable();
  }

  return {};
}

Expect<void> pushType(Runtime::Instance::ComponentInstance &Comp,
                      std::vector<InterfaceType> &Types, const ValueType &VT) {
  // notice that we might need to convert one type to multiple types, and hence,
  // we must let this function control the vector need to be modified.
  if (std::holds_alternative<PrimValType>(VT)) {
    switch (std::get<PrimValType>(VT)) {
    case PrimValType::Bool:
    case PrimValType::Char:
    case PrimValType::S8:
    case PrimValType::U8:
      Types.push_back(InterfaceType(TypeCode::I8));
      break;
    case PrimValType::S16:
    case PrimValType::U16:
      Types.push_back(InterfaceType(TypeCode::I16));
      break;
    case PrimValType::S32:
    case PrimValType::U32:
      Types.push_back(InterfaceType(TypeCode::I32));
      break;
    case PrimValType::S64:
    case PrimValType::U64:
      Types.push_back(InterfaceType(TypeCode::I64));
      break;
    case PrimValType::Float32:
      Types.push_back(InterfaceType(TypeCode::F32));
      break;
    case PrimValType::Float64:
      Types.push_back(InterfaceType(TypeCode::F64));
      break;
    case PrimValType::String:
      Types.push_back(InterfaceType(TypeCode::String));
      break;
    }
  } else if (std::holds_alternative<TypeIndex>(VT)) {
    EXPECTED_TRY(auto Ty, Comp.getType(std::get<TypeIndex>(VT)));
    pushDefType(Comp, Types, Ty);
  }

  return {};
}

Expect<AST::Component::FunctionType>
liftFlattenType(Runtime::Instance::ComponentInstance &Comp,
                const FuncType &DT) {
  Expect<void> Res;

  std::vector<InterfaceType> ParamTypes{};
  for (const auto &P : DT.getParamList()) {
    Res = pushType(Comp, ParamTypes, P.getValType());
    if (!Res) {
      return Unexpect(Res);
    }
  }

  std::vector<InterfaceType> ResultTypes{};
  auto Func = [&](auto &&Type) -> Expect<void> {
    using T = std::decay_t<decltype(Type)>;
    if constexpr (std::is_same_v<T, ValueType>) {
      Res = pushType(Comp, ResultTypes, Type);
      if (!Res) {
        return Unexpect(Res);
      }
    } else if constexpr (std::is_same_v<T, std::vector<LabelValType>>) {
      for (const auto &R : Type) {
        Res = pushType(Comp, ResultTypes, R.getValType());
        if (!Res) {
          return Unexpect(Res);
        }
      }
    }
    return {};
  };
  EXPECTED_TRY(std::visit(Func, DT.getResultList()));

  return AST::Component::FunctionType(ParamTypes, ResultTypes);
}

InterfaceType discriminantType(size_t N) {
  assert(0 < N && N < 4294967296);
  uint8_t E = static_cast<uint8_t>(std::ceil(log2(N) / 8));
  switch (E) {
  case 0:
    return InterfaceType(TypeCode::U8);
  case 1:
    return InterfaceType(TypeCode::U8);
  case 2:
    return InterfaceType(TypeCode::U16);
  case 3:
    return InterfaceType(TypeCode::U32);
  default:
    assumingUnreachable();
  }
}

InterfaceType despecialize(InterfaceType Ty) {
  switch (Ty.getCode()) {
  case TypeCode::Enum:
    return Ty;
  case TypeCode::Option: {
    auto T = Ty.getArgs()[0];
    T.setCaseName("some");
    return InterfaceType(TypeCode::Variant,
                         {InterfaceType(TypeCode::Tuple, "none"), T});
  }
  case TypeCode::Result: {
    auto Ok = Ty.getArgs()[0];
    auto Err = Ty.getArgs()[1];
    Ok.setCaseName("ok");
    Err.setCaseName("error");
    return InterfaceType(TypeCode::Variant, {Ok, Err});
  }
  default:
    return Ty;
  }
}

std::vector<ValType> flattenType(const InterfaceType &SpecialTy);

ValType join(ValType A, ValType B) {
  if (A == B) {
    return A;
  } else if ((A.getCode() == TypeCode::I32 && B.getCode() == TypeCode::F32) ||
             (A.getCode() == TypeCode::F32 && B.getCode() == TypeCode::I32)) {
    return ValType(TypeCode::I32);
  } else {
    return ValType(TypeCode::I64);
  }
}

std::vector<ValType> flattenVariant(Span<const InterfaceType> Cases) {
  std::vector<ValType> Flat{};

  for (auto C : Cases) {
    if (!C.isNone()) {
      std::vector<ValType> FTs = flattenType(C);
      for (size_t I = 0; I < FTs.size(); ++I) {
        auto FT = FTs[I];
        if (I < Flat.size()) {
          Flat[I] = join(Flat[I], FT);
        } else {
          Flat.push_back(FT);
        }
      }
    }
  }

  auto InTy = discriminantType(Cases.size());
  std::vector<ValType> Output = flattenType(InTy);
  Output.insert(Output.end(), Flat.begin(), Flat.end());
  return Output;
}

std::vector<ValType> flattenType(const InterfaceType &SpecialTy) {
  auto Ty = despecialize(SpecialTy);
  switch (Ty.getCode()) {
  case TypeCode::Bool:
  case TypeCode::U8:
  case TypeCode::U16:
  case TypeCode::U32:
  case TypeCode::I8:
  case TypeCode::I16:
  case TypeCode::I32:
    return {TypeCode::I32};
  case TypeCode::U64:
  case TypeCode::I64:
    return {TypeCode::I64};
  case TypeCode::String:
    return {TypeCode::I32, TypeCode::I32};
  case TypeCode::List: {
    // TODO:
    // if maybe_length is not None:
    //     return flatten_type(elem_type) * maybe_length
    return {TypeCode::I32, TypeCode::I32};
  }
  case TypeCode::Record: {
    std::vector<ValType> Flat;
    for (auto FT : Ty.getArgs()) {
      auto V = flattenType(FT);
      Flat.insert(Flat.end(), V.begin(), V.end());
    }
    return Flat;
  }
  case TypeCode::Variant: {
    return flattenVariant(Ty.getArgs());
  }
  default:
    return {Ty.getValType()};
  }
}

} // namespace

class LiftTrans : public WasmEdge::Runtime::Component::HostFunctionBase {
public:
  LiftTrans(Executor *Exec, const AST::Component::FuncType &DefinedType,
            Instance::FunctionInstance *CoreFunc, Instance::MemoryInstance *M,
            Instance::FunctionInstance *R,
            Runtime::Instance::ComponentInstance &Comp)
      : HostFunctionBase(), Exec(Exec), LowerFunc(CoreFunc), Memory(M),
        Realloc(R) {
    // The convert is simply let component type to internal type.
    auto RFuncType = liftFlattenType(Comp, DefinedType);
    if (!RFuncType) {
      spdlog::error("failed to lift function");
    }
    FuncType = *RFuncType;
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

std::unique_ptr<Instance::Component::FunctionInstance> Executor::lifting(
    Runtime::Instance::ComponentInstance &Comp, const FuncType &FuncType,
    Instance::FunctionInstance *CoreFunc, Instance::MemoryInstance *Memory,
    Instance::FunctionInstance *Realloc) {
  return std::make_unique<Instance::Component::FunctionInstance>(
      std::make_unique<LiftTrans>(this, FuncType, CoreFunc, Memory, Realloc,
                                  Comp));
}

class LowerTrans : public HostFunctionBase {
public:
  LowerTrans(Executor *Exec, Instance::Component::FunctionInstance *Func,
             Instance::MemoryInstance *Memory,
             Instance::FunctionInstance *Realloc)
      : HostFunctionBase(0), Exec(Exec), HigherFunc(Func), Memory(Memory),
        Realloc(Realloc) {
    auto &HigherType = HigherFunc->getFuncType();
    flattenFunctype(HigherType);
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
  // This is flatten function type for lowering, and hence it's about converting
  // `InterfaceType` to `ValType`
  //
  // TODO: opts.sync was read from canonical lift/lower command
  void flattenFunctype(const AST::Component::FunctionType &HigherType) {
    auto &FuncType = DefType.getCompositeType().getFuncType();

    std::vector<ValType> FlatParams{};
    std::vector<ValType> FlatResults{};
    for (auto &ParamTy : HigherType.getParamTypes()) {
      auto V = flattenType(ParamTy);
      FlatParams.insert(FlatParams.end(), V.begin(), V.end());
    }
    for (auto &ReturnTy : HigherType.getReturnTypes()) {
      auto V = flattenType(ReturnTy);
      FlatResults.insert(FlatResults.end(), V.begin(), V.end());
    }
    if (FlatParams.size() > MAX_FLAT_PARAMS) {
      FlatParams.clear();
      FlatParams.push_back(ValType(TypeCode::I32));
    }
    // context: lower
    if (FlatResults.size() > MAX_FLAT_RESULTS) {
      FlatParams.push_back(ValType(TypeCode::I32));
      FlatResults.clear();
    }

    FuncType.getParamTypes() = FlatParams;
    FuncType.getReturnTypes() = FlatResults;
    spdlog::info("lower: {}"sv, FuncType);
  }

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
  return std::make_unique<Instance::FunctionInstance>(
      std::make_unique<LowerTrans>(this, Func, Memory, Realloc));
}

class ResourceDropHostFunction
    : public WasmEdge::Runtime::Component::HostFunctionBase {
public:
  ResourceDropHostFunction(Executor *E, AST::Component::ResourceType &RT,
                           Runtime::Instance::ComponentInstance &C)
      : Exec{E}, RTyp{RT}, Comp{C} {
    std::vector<InterfaceType> ParamTypes{InterfaceType(TypeCode::I32)};
    // NOTE: resource destructor only use type `i32`
    // 1. at sync mode: [i32] -> []
    // 2. at async mode: [i32] -> [i32]
    // for now, we ignore async case, to simplify our program here, but at
    // future shall make a full concept supportings.
    std::vector<InterfaceType> ResultTypes{};
    FuncType = AST::Component::FunctionType(ParamTypes, ResultTypes);
  }

  Expect<void> run(Span<const ValInterface> Args,
                   Span<ValInterface> /* Rets */) {
    if (Args.size() != 1 || !std::holds_alternative<ValVariant>(Args[0])) {
      spdlog::info("bad argument");
      return Unexpect(ErrCode::Value::ResourceDropArgument);
    }

    auto ResourceIdx = std::get<ValVariant>(Args[0]).get<uint32_t>();

    std::shared_ptr<Runtime::Instance::ResourceHandle> Handle =
        Comp.removeResource(ResourceIdx);
    if (Handle->isOwn()) {
      // TODO: assert borrowScope is None
      // TODO: trap lendCount != 0

      // TODO: Comp is RTyp.impl
      if (true) {
        if (*RTyp.getDestructor()) {
          auto Idx = *RTyp.getDestructor();
          auto F = Comp.getFunctionInstance(Idx);
          auto Arg = ValInterface(ValVariant(Handle->getRep()));
          Exec->invoke(F, {Arg}, {InterfaceType(TypeCode::I32)});
        }
      } else {
        if (*RTyp.getDestructor()) {
          // TODO
          // caller_opts = CanonicalOptions(sync = sync)
          // callee_opts = CanonicalOptions(sync = rt.dtor_sync, callback =
          // rt.dtor_callback)
          // ft = FuncType([U32Type()],[])
          // callee = partial(canon_lift, callee_opts, rt.impl, ft, rt.dtor)
          // flat_results = await canon_lower(caller_opts, ft, callee, task,
          // [h.rep])
        } else {
          // task.trap_if_on_the_stack(rt.impl)
        }
      }
    } else {
      // TODO: Handle.borrowScope.todo -= 1
    }

    return {};
  }

private:
  Executor *Exec;
  AST::Component::ResourceType &RTyp;
  Runtime::Instance::ComponentInstance &Comp;
};

std::unique_ptr<Instance::Component::FunctionInstance>
Executor::resourceDrop(AST::Component::ResourceType &RTyp,
                       Runtime::Instance::ComponentInstance &CompInst) {
  return std::make_unique<Instance::Component::FunctionInstance>(
      std::make_unique<ResourceDropHostFunction>(this, RTyp, CompInst));
}

class CanonOptionVisitor {
private:
  Executor &ThisExecutor;
  Runtime::Instance::ComponentInstance &CompInst;

public:
  CanonOptionVisitor(Executor &E, Runtime::Instance::ComponentInstance &CInst)
      : ThisExecutor{E}, CompInst{CInst} {}

  // lift wrap a core wasm function to a component function, with proper
  // modification about canonical ABI.
  Expect<void> operator()(const Lift &L) {
    const auto &Opts = L.getOptions();

    Runtime::Instance::MemoryInstance *Mem = nullptr;
    Runtime::Instance::FunctionInstance *ReallocFunc = nullptr;
    for (auto &Opt : Opts) {
      if (std::holds_alternative<StringEncoding>(Opt)) {
        switch (std::get<StringEncoding>(Opt)) {
        case StringEncoding::UTF8:
          spdlog::warn("ignore utf8");
          break;
        case StringEncoding::UTF16:
          spdlog::warn("ignore utf16");
          break;
        case StringEncoding::Latin1:
          spdlog::warn("ignore latin1");
          break;
        default:
          assumingUnreachable();
        }
      } else if (std::holds_alternative<Memory>(Opt)) {
        auto MemIdx = std::get<Memory>(Opt).getMemIndex();
        auto RMem = CompInst.getCoreMemoryInstance(MemIdx);
        if (!RMem) {
          return Unexpect(RMem);
        }
        Mem = *RMem;
      } else if (std::holds_alternative<Realloc>(Opt)) {
        auto Res = CompInst.getCoreFunctionInstance(
            std::get<Realloc>(Opt).getFuncIndex());
        if (!Res) {
          return Unexpect(Res);
        }
        ReallocFunc = *Res;
      } else if (std::holds_alternative<PostReturn>(Opt)) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Canon));
        return Unexpect(ErrCode::Value::InvalidCanonOption);
      }
    }

    EXPECTED_TRY(auto const AstFuncType,
                 CompInst.getType(L.getFuncTypeIndex()));
    if (unlikely(!std::holds_alternative<FuncType>(AstFuncType))) {
      // It doesn't make sense if one tries to lift an instance not a
      // function, so unlikely happen.
      spdlog::error("cannot lift a non-function"sv);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Canon));
      return Unexpect(ErrCode::Value::InvalidCanonOption);
    }

    EXPECTED_TRY(auto CoreFuncInst,
                 CompInst.getCoreFunctionInstance(L.getCoreFuncIndex()));
    CompInst.addFunctionInstance(
        ThisExecutor.lifting(CompInst, std::get<FuncType>(AstFuncType),
                             CoreFuncInst, Mem, ReallocFunc));

    return {};
  }

  // lower sends a component function to a core wasm function, with proper
  // modification about canonical ABI.
  Expect<void> operator()(const Lower &L) {
    Runtime::Instance::MemoryInstance *Mem = nullptr;
    Runtime::Instance::FunctionInstance *ReallocFunc = nullptr;

    const auto &Opts = L.getOptions();
    for (auto &Opt : Opts) {
      if (std::holds_alternative<StringEncoding>(Opt)) {
        switch (std::get<StringEncoding>(Opt)) {
        case StringEncoding::UTF8:
          spdlog::warn("ignore utf8");
          break;
        case StringEncoding::UTF16:
          spdlog::warn("ignore utf16");
          break;
        case StringEncoding::Latin1:
          spdlog::warn("ignore latin1");
          break;
        default:
          assumingUnreachable();
        }
      } else if (std::holds_alternative<Memory>(Opt)) {
        auto MemIdx = std::get<Memory>(Opt).getMemIndex();
        EXPECTED_TRY(Mem, CompInst.getCoreMemoryInstance(MemIdx));
      } else if (std::holds_alternative<Realloc>(Opt)) {
        EXPECTED_TRY(ReallocFunc, CompInst.getCoreFunctionInstance(
                                      std::get<Realloc>(Opt).getFuncIndex()));
      } else if (std::holds_alternative<PostReturn>(Opt)) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Canon));
        return Unexpect(ErrCode::Value::InvalidCanonOption);
      }
    }

    // lower sends a component function to a core wasm function, with proper
    // modification about canonical ABI.
    auto *FuncInst = CompInst.getFunctionInstance(L.getFuncIndex());
    CompInst.addCoreFunctionInstance(
        ThisExecutor.lowering(FuncInst, Mem, ReallocFunc));

    return {};
  }

  Expect<void> operator()(const ResourceNew &RNew) {
    auto TypIdx = RNew.getTypeIndex();
    EXPECTED_TRY(auto const Typ, CompInst.getType(TypIdx));
    if (!std::holds_alternative<ResourceType>(Typ)) {
      spdlog::error(
          "resource.new cannot instantiate a deftype that's not a resource.");
      return Unexpect(ErrCode::Value::InvalidCanonOption);
    }
    auto ResourceTyp = std::get<ResourceType>(Typ);
    spdlog::info("get {}", ResourceTyp);
    spdlog::warn("resource.new is not supported yet"sv);

    return {};
  }

  Expect<void> operator()(const ResourceDrop &RDrop) {
    auto TypIdx = RDrop.getTypeIndex();
    EXPECTED_TRY(auto const Typ, CompInst.getType(TypIdx));
    if (!std::holds_alternative<ResourceType>(Typ)) {
      spdlog::error("resource.drop cannot instantiate a deftype that's not a "
                    "resource.");
      return Unexpect(ErrCode::Value::InvalidCanonOption);
    }
    auto ResourceTyp = std::get<ResourceType>(Typ);
    auto Drop = ThisExecutor.resourceDrop(ResourceTyp, CompInst);
    Instance::Component::FunctionInstance *F = Drop.get();
    CompInst.addCoreFunctionInstance(
        ThisExecutor.lowering(F, nullptr, nullptr));

    return {};
  }

  Expect<void> operator()(const ResourceRep &) {
    spdlog::warn("resource.rep is not supported yet"sv);
    return Unexpect(ErrCode::Value::InvalidCanonOption);
  }
};

Expect<void>
Executor::instantiate(Runtime::StoreManager &,
                      Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::CanonSection &CanonSec) {
  for (const Canon &C : CanonSec.getContent()) {
    auto V = CanonOptionVisitor{*this, CompInst};
    auto Res = std::visit(V, C);
    if (!Res) {
      return Unexpect(Res);
    }
  }

  return {};
}

} // namespace Executor
} // namespace WasmEdge
