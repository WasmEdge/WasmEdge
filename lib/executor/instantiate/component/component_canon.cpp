// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <string_view>

namespace WasmEdge {
namespace Executor {

using namespace std::literals;

std::vector<ValVariant> Executor::convValsToCoreWASM(
    Span<const ComponentValVariant> Vals, Span<const ComponentValType> ValTypes,
    Runtime::Instance::FunctionInstance *RFuncInst,
    Runtime::Instance::MemoryInstance *MemInst) noexcept {
  uint32_t I = 0;
  std::vector<ValVariant> CoreVals;
  for (const auto &Type : ValTypes) {
    switch (Type.getCode()) {
    case ComponentTypeCode::String: {
      assuming(RFuncInst != nullptr);
      assuming(MemInst != nullptr);
      std::string_view Str = std::get<std::string>(Vals[I++]);
      uint32_t StrSize = static_cast<uint32_t>(Str.size());
      std::vector<ValVariant> ReallocArgs{ValVariant(0), ValVariant(0),
                                          ValVariant(0), ValVariant(StrSize)};
      std::vector<ValType> ReallocTypes =
          RFuncInst->getFuncType().getParamTypes();
      auto AllocRes = invoke(RFuncInst, ReallocArgs, ReallocTypes);
      if (AllocRes) {
        ValVariant PtrInMem = (*AllocRes)[0].first;
        MemInst->setBytes(std::vector<Byte>{Str.begin(), Str.end()},
                          PtrInMem.get<uint32_t>(), 0,
                          static_cast<uint32_t>(Str.size()));
        CoreVals.push_back(PtrInMem);
        CoreVals.push_back(StrSize);
      } else {
        CoreVals.push_back(0);
        CoreVals.push_back(0);
      }
      break;
    }
    case ComponentTypeCode::TypeIndex: {
      // TODO: COMPONENT - Not implemented.
      break;
    }
    default: {
      // Other types don't need conversion
      const ValVariant &Val = std::get<ValVariant>(Vals[I++]);
      CoreVals.push_back(Val);
      break;
    }
    }
  }
  return CoreVals;
}

std::vector<std::pair<ComponentValVariant, ComponentValType>>
Executor::convValsToComponent(
    Span<const std::pair<ValVariant, ValType>> CoreVals,
    Span<const ComponentValType> ValTypes,
    Runtime::Instance::MemoryInstance *MemInst) noexcept {
  uint32_t I = 0;
  std::vector<std::pair<ComponentValVariant, ComponentValType>> Vals;
  for (const auto &Type : ValTypes) {
    switch (Type.getCode()) {
    case ComponentTypeCode::String: {
      auto Off = CoreVals[I++].first.get<uint32_t>();
      auto Size = CoreVals[I++].first.get<uint32_t>();
      auto Str = MemInst->getStringView(Off, Size);
      Vals.emplace_back(std::string(Str), Type);
      break;
    }
    case ComponentTypeCode::TypeIndex: {
      // TODO: COMPONENT - Not implemented.
      break;
    }
    default: {
      Vals.emplace_back(CoreVals[I++].first, Type);
      break;
    }
    }
  }
  return Vals;
}

Expect<void>
Executor::instantiate(Runtime::Instance::ComponentInstance &CompInst,
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
          MemInst = CompInst.getCoreMemory(Opt.getIndex());
          break;
        case AST::Component::CanonOpt::OptCode::Realloc:
          ReallocFunc = CompInst.getCoreFunction(Opt.getIndex());
          break;
        case AST::Component::CanonOpt::OptCode::PostReturn:
        case AST::Component::CanonOpt::OptCode::Async:
          // TODO: incomplete validation of these cases.
        default:
          assumingUnreachable();
        }
      }

      const auto *DType = CompInst.getType(Canon.getTargetIndex());
      if (unlikely(!DType->isFuncType())) {
        // It doesn't make sense if one tries to lift an instance not a
        // function, so unlikely happen.
        spdlog::error(ErrCode::Value::InvalidCanonOption);
        spdlog::error("    Cannot lift a non-function"sv);
        return Unexpect(ErrCode::Value::InvalidCanonOption);
      }
      auto *FuncInst = CompInst.getCoreFunction(Canon.getIndex());
      CompInst.addFunction(
          std::make_unique<Runtime::Instance::Component::FunctionInstance>(
              DType->getFuncType(), FuncInst, MemInst, ReallocFunc));
      break;
    }
    case AST::Component::Canonical::OpCode::Lower: {
      // lower sends a component function to a core wasm function, with proper
      // modification about canonical ABI.

      // TODO: COMPONENT - Currently the component functions are from `lifting`,
      // therefore there is a core function instance under the component
      // function instance. Maybe this implementation should be fixed in the
      // future.
      /*
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
          MemInst = CompInst.getCoreMemory(Opt.getIndex());
          break;
        case AST::Component::CanonOpt::OptCode::Realloc:
          ReallocFunc = CompInst.getCoreFunction(Opt.getIndex());
          break;
        case AST::Component::CanonOpt::OptCode::PostReturn:
        case AST::Component::CanonOpt::OptCode::Async:
          // TODO: incomplete validation of these cases.
        default:
          assumingUnreachable();
        }
      }
      */

      auto *FuncInst = CompInst.getFunction(Canon.getIndex());
      auto *CoreFuncInst = FuncInst->getLowerFunction();
      CompInst.addCoreFunction(CoreFuncInst);
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
