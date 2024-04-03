#include "ast/component/instance.h"
#include "ast/module.h"
#include "common/errcode.h"
#include "executor/executor.h"

#include "runtime/instance/module.h"

#include <string_view>
#include <variant>

namespace WasmEdge {
namespace Executor {

using namespace AST::Component;
using namespace Runtime;

std::string funcType_fmt(const WasmEdge::AST::FunctionType &FT) {
  std::string TyStr{};
  std::ostringstream TyInfo;
  for (auto P : FT.getParamTypes()) {
    TyInfo << fmt::format("{} ", P);
  }
  TyInfo << "-> ";
  for (auto R : FT.getReturnTypes()) {
    TyInfo << fmt::format("{} ", R);
  }
  return TyInfo.str();
}

class LowerTrans : public HostFunctionBase {
public:
  LowerTrans(Executor *Exec, Instance::FunctionInstance *Func,
             Instance::MemoryInstance *Memory,
             Instance::FunctionInstance *Realloc)
      : HostFunctionBase(0), Exec(Exec), HigherFunc(Func), Memory(Memory),
        Realloc(Realloc) {
    const auto HigherType = HigherFunc->getFuncType();

    auto &CompositeType = DefType.getCompositeType();
    auto &FuncType = CompositeType.getFuncType();
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
                   Span<ValVariant> Rets) {
    auto HigherFuncType = HigherFunc->getFuncType();

    uint32_t PI = 0;
    std::vector<const ValVariant> HigherArgs{};
    for (auto &ParamTy : HigherFuncType.getParamTypes()) {
      switch (ParamTy.getCode()) {
      case TypeCode::String: {
        auto Idx = Args[PI++];
        auto Len = Args[PI++];
        std::string_view V =
            Memory->getStringView(Idx.get<uint32_t>(), Len.get<uint32_t>());
        std::string S = {V.begin(), V.end()};
        HigherArgs.emplace_back(
            StrVariant(new Instance::StringInstance(std::move(S))));
        break;
      }
      default:
        // usual type has no need conversion
        HigherArgs.emplace_back(Args[PI++]);
        break;
      }
    }

    auto Res =
        Exec->invoke(HigherFunc, HigherArgs, HigherFuncType.getReturnTypes());
    if (!Res) {
      return Unexpect(Res);
    }

    uint32_t RI = 0;
    for (auto &[RetVal, RetTy] : *Res) {
      switch (RetTy.getCode()) {
      case TypeCode::String: {
        auto Ptr = RetVal.get<StrVariant>().getPtr();
        auto const Str = Ptr->getString();
        ValVariant StrSize = static_cast<uint32_t>(Str.size());

        std::vector<const ValVariant> ReallocArgs{0, 0, 0, StrSize};
        auto RPtr = Exec->invoke(Realloc, ReallocArgs,
                                 Realloc->getFuncType().getReturnTypes());
        if (!RPtr) {
          return Unexpect(RPtr);
        }
        // notice a higher function is expected only returning one result
        ValVariant V = (*RPtr)[0].first;
        Rets[RI++] = V;
        Rets[RI++] = StrSize;
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

      auto L = std::get<Lift>(C);

      auto *FuncInst = CompInst.getCoreFunctionInstance(L.getCoreFuncIndex());

      auto &Opts = L.getOptions();
      // TODO
      for (auto &Opt : Opts) {
        if (std::holds_alternative<StringEncoding>(Opt)) {
          spdlog::warn("incomplete canonical option `string-encoding`");
        } else if (std::holds_alternative<Memory>(Opt)) {
          spdlog::warn("incomplete canonical option `memory`");
        } else if (std::holds_alternative<Realloc>(Opt)) {
          spdlog::warn("incomplete canonical option `realloc`");
        } else if (std::holds_alternative<PostReturn>(Opt)) {
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Canon));
          return Unexpect(ErrCode::Value::InvalidCanonOption);
        }
      }

      CompInst.addFunctionInstance(FuncInst);
    } else if (std::holds_alternative<Lower>(C)) {
      // lower sends a component function to a core wasm function, with proper
      // modification about canonical ABI.
      auto L = std::get<Lower>(C);

      Runtime::Instance::MemoryInstance *Mem;
      Runtime::Instance::FunctionInstance *ReallocFunc;
      auto &Opts = L.getOptions();
      for (auto &Opt : Opts) {
        if (std::holds_alternative<StringEncoding>(Opt)) {
          spdlog::warn("incomplete canonical option `string-encoding`");
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
      spdlog::warn("resource is not supported yet");
    } else if (std::holds_alternative<ResourceDrop>(C)) {
      spdlog::warn("resource is not supported yet");
    } else if (std::holds_alternative<ResourceRep>(C)) {
      spdlog::warn("resource is not supported yet");
    }
  }

  return {};
}

} // namespace Executor
} // namespace WasmEdge
