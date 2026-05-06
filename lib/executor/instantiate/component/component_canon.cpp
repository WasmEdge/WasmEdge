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
      // Realloc signature: (original_ptr, original_size, alignment, new_size)
      // -> ptr
      // For new allocation: original_ptr=0, original_size=0, alignment=1 (for
      // strings)
      std::vector<ValVariant> ReallocArgs{ValVariant(0), ValVariant(0),
                                          ValVariant(1), ValVariant(StrSize)};
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
      // lift wraps a core wasm function to a component function with
      // canonical ABI conversion.
      Runtime::Instance::Component::CanonicalOptions CanonOpts;

      // Parse canonical options
      const auto &Opts = Canon.getOptions();
      for (auto &Opt : Opts) {
        switch (Opt.getCode()) {
        case AST::Component::CanonOpt::OptCode::Encode_UTF8:
          CanonOpts.StringEncoding =
              AST::Component::CanonOpt::OptCode::Encode_UTF8;
          break;
        case AST::Component::CanonOpt::OptCode::Encode_UTF16:
          CanonOpts.StringEncoding =
              AST::Component::CanonOpt::OptCode::Encode_UTF16;
          break;
        case AST::Component::CanonOpt::OptCode::Encode_Latin1:
          CanonOpts.StringEncoding =
              AST::Component::CanonOpt::OptCode::Encode_Latin1;
          break;
        case AST::Component::CanonOpt::OptCode::Memory:
          CanonOpts.Memory = CompInst.getCoreMemory(Opt.getIndex());
          break;
        case AST::Component::CanonOpt::OptCode::Realloc:
          CanonOpts.Realloc = CompInst.getCoreFunction(Opt.getIndex());
          break;
        case AST::Component::CanonOpt::OptCode::PostReturn:
          CanonOpts.PostReturn = CompInst.getCoreFunction(Opt.getIndex());
          break;
        case AST::Component::CanonOpt::OptCode::Async:
          CanonOpts.IsAsync = true;
          break;
        case AST::Component::CanonOpt::OptCode::Callback:
          // TODO: COMPONENT - Implement callback support
          spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
          spdlog::error("    callback option not yet implemented"sv);
          return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
        default:
          assumingUnreachable();
        }
      }

      // Get the component function type
      const auto *DType = CompInst.getType(Canon.getTargetIndex());
      if (unlikely(!DType->isFuncType())) {
        spdlog::error(ErrCode::Value::InvalidCanonOption);
        spdlog::error("    Cannot lift a non-function"sv);
        return Unexpect(ErrCode::Value::InvalidCanonOption);
      }

      // Get the core function to lift
      auto *CoreFuncInst = CompInst.getCoreFunction(Canon.getIndex());

      // Create lifted component function instance
      CompInst.addFunction(
          std::make_unique<Runtime::Instance::Component::FunctionInstance>(
              DType->getFuncType(), CoreFuncInst, CanonOpts));
      break;
    }
    case AST::Component::Canonical::OpCode::Lower: {
      // lower converts a component function to a core wasm function with
      // canonical ABI conversion.
      Runtime::Instance::Component::CanonicalOptions CanonOpts;

      // Parse canonical options
      const auto &Opts = Canon.getOptions();
      for (auto &Opt : Opts) {
        switch (Opt.getCode()) {
        case AST::Component::CanonOpt::OptCode::Encode_UTF8:
          CanonOpts.StringEncoding =
              AST::Component::CanonOpt::OptCode::Encode_UTF8;
          break;
        case AST::Component::CanonOpt::OptCode::Encode_UTF16:
          CanonOpts.StringEncoding =
              AST::Component::CanonOpt::OptCode::Encode_UTF16;
          break;
        case AST::Component::CanonOpt::OptCode::Encode_Latin1:
          CanonOpts.StringEncoding =
              AST::Component::CanonOpt::OptCode::Encode_Latin1;
          break;
        case AST::Component::CanonOpt::OptCode::Memory:
          CanonOpts.Memory = CompInst.getCoreMemory(Opt.getIndex());
          break;
        case AST::Component::CanonOpt::OptCode::Realloc:
          CanonOpts.Realloc = CompInst.getCoreFunction(Opt.getIndex());
          break;
        case AST::Component::CanonOpt::OptCode::PostReturn:
          CanonOpts.PostReturn = CompInst.getCoreFunction(Opt.getIndex());
          break;
        case AST::Component::CanonOpt::OptCode::Async:
          CanonOpts.IsAsync = true;
          break;
        case AST::Component::CanonOpt::OptCode::Callback:
          // TODO: COMPONENT - Implement callback support
          spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
          spdlog::error("    callback option not yet implemented"sv);
          return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
        default:
          assumingUnreachable();
        }
      }

      // Get the component function
      auto *CompFuncInst = CompInst.getFunction(Canon.getIndex());

      // For lifted functions, extract the underlying core function
      // For host functions, this operation is not yet supported
      if (CompFuncInst->isLifted()) {
        auto *CoreFuncInst = CompFuncInst->getLowerFunction();
        CompInst.addCoreFunction(CoreFuncInst);
      } else {
        // TODO: COMPONENT - Implement lowering of host functions
        spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
        spdlog::error("    lowering host functions not yet implemented"sv);
        return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
      }
      break;
    }
    case AST::Component::Canonical::OpCode::Resource__new:
    case AST::Component::Canonical::OpCode::Resource__drop:
    case AST::Component::Canonical::OpCode::Resource__rep:
    case AST::Component::Canonical::OpCode::Resource__drop_async:
    case AST::Component::Canonical::OpCode::Task__return:
    case AST::Component::Canonical::OpCode::Task__cancel:
      // TODO: COMPONENT - Implement resource and task operations
      spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
      spdlog::error("    resource/task canonical operations not yet "
                    "implemented"sv);
      return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
    default:
      spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
      spdlog::error("    unsupported canonical operation"sv);
      return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
    }
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
