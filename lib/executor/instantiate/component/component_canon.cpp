// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <cstdint>
#include <optional>
#include <string_view>

namespace WasmEdge {
namespace Executor {

using namespace std::literals;

namespace {

constexpr uint32_t MaxFlatResults = 1;

std::optional<uint32_t> flatSize(ComponentTypeCode TC) noexcept {
  switch (TC) {
  case ComponentTypeCode::Bool:
  case ComponentTypeCode::U8:
  case ComponentTypeCode::U16:
  case ComponentTypeCode::U32:
  case ComponentTypeCode::U64:
  case ComponentTypeCode::S8:
  case ComponentTypeCode::S16:
  case ComponentTypeCode::S32:
  case ComponentTypeCode::S64:
  case ComponentTypeCode::F32:
  case ComponentTypeCode::F64:
  case ComponentTypeCode::Char:
  case ComponentTypeCode::Flags:
    return 1u;
  case ComponentTypeCode::String:
    return 2u;
  default:
    return std::nullopt;
  }
}

} // namespace

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
      // realloc(old_ptr=0, old_size=0, alignment=1, new_size=StrSize)
      // Alignment = 1 for UTF-8 strings (CanonicalABI spec L2432).
      // TODO: When UTF-16 or Latin1+UTF16 encoding support is added,
      // alignment should be 2 (CanonicalABI spec L2438, L2446).
      std::vector<ValVariant> ReallocArgs{ValVariant(0), ValVariant(0),
                                          ValVariant(1), ValVariant(StrSize)};
      std::vector<ValType> ReallocTypes =
          RFuncInst->getFuncType().getParamTypes();
      auto AllocRes = invoke(RFuncInst, ReallocArgs, ReallocTypes);
      // The 'realloc' core function signature is not yet checked during
      // validation (GAP-C-5b). It must return exactly one i32 pointer; a
      // component declaring a 'realloc' with no result would otherwise make
      // (*AllocRes)[0] an out-of-bounds access on an empty vector. Guard the
      // result count, and propagate the setBytes failure instead of silently
      // handing the guest a (ptr, size) pair for bytes that were never copied.
      if (AllocRes && !AllocRes->empty()) {
        ValVariant PtrInMem = (*AllocRes)[0].first;
        if (auto Res = MemInst->setBytes(
                std::vector<Byte>{Str.begin(), Str.end()},
                PtrInMem.get<uint32_t>(), 0, static_cast<uint32_t>(Str.size()));
            Res) {
          CoreVals.push_back(PtrInMem);
          CoreVals.push_back(StrSize);
        } else {
          CoreVals.push_back(0);
          CoreVals.push_back(0);
        }
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
      // Other types do not need conversion.
      const ValVariant &Val = std::get<ValVariant>(Vals[I++]);
      CoreVals.push_back(Val);
      break;
    }
    }
  }
  return CoreVals;
}

Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>
Executor::convValsToComponent(
    Span<const std::pair<ValVariant, ValType>> CoreVals,
    Span<const ComponentValType> ValTypes,
    Runtime::Instance::MemoryInstance *MemInst) {
  uint32_t FlatCount = 0;
  for (const auto &Type : ValTypes) {
    auto Size = flatSize(Type.getCode());
    if (!Size.has_value()) {
      spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
      spdlog::error(
          "    canonical lifting of component type 0x{:02x} not implemented"sv,
          static_cast<uint8_t>(Type.getCode()));
      return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
    }
    FlatCount += *Size;
  }

  if (FlatCount > MaxFlatResults) {
    spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
    spdlog::error(
        "    canonical lifting via return-area pointer (flat count {} > {}) "
        "not implemented"sv,
        FlatCount, MaxFlatResults);
    return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
  }

  uint32_t I = 0;
  std::vector<std::pair<ComponentValVariant, ComponentValType>> Vals;
  Vals.reserve(ValTypes.size());
  for (const auto &Type : ValTypes) {
    switch (Type.getCode()) {
    case ComponentTypeCode::String: {
      assuming(MemInst != nullptr);
      auto Off = CoreVals[I++].first.get<uint32_t>();
      auto Size = CoreVals[I++].first.get<uint32_t>();
      if (unlikely(!MemInst->checkAccessBound(Off, Size))) {
        spdlog::error(ErrCode::Value::MemoryOutOfBounds);
        spdlog::error("    string pointer/length out of bounds of memory"sv);
        return Unexpect(ErrCode::Value::MemoryOutOfBounds);
      }
      auto Str = MemInst->getStringView(Off, Size);
      Vals.emplace_back(std::string(Str), Type);
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
    case ComponentCanonOpCode::Lift: {
      // Lift wraps a core Wasm function to a component function with proper
      // canonical ABI modification.
      const auto &Opts = Canon.getOptions();
      Runtime::Instance::MemoryInstance *MemInst = nullptr;
      Runtime::Instance::FunctionInstance *ReallocFunc = nullptr;
      for (auto &Opt : Opts) {
        switch (Opt.getCode()) {
        case ComponentCanonOptCode::Encode_UTF8:
        case ComponentCanonOptCode::Encode_UTF16:
        case ComponentCanonOptCode::Encode_Latin1:
          spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
          spdlog::error("    incomplete canonincal options"sv);
          return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
        case ComponentCanonOptCode::Memory:
          MemInst = CompInst.getCoreMemory(Opt.getIndex());
          break;
        case ComponentCanonOptCode::Realloc:
          ReallocFunc = CompInst.getCoreFunction(Opt.getIndex());
          break;
        case ComponentCanonOptCode::PostReturn:
        case ComponentCanonOptCode::Async:
          // TODO: incomplete validation of these cases.
        default:
          assumingUnreachable();
        }
      }

      const auto *DType = CompInst.getType(Canon.getTargetIndex());
      if (unlikely(!DType->isFuncType())) {
        // It does not make sense to lift an instance that is not a function, so
        // this is unlikely to happen.
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
    case ComponentCanonOpCode::Lower: {
      // Lower sends a component function to a core Wasm function with proper
      // canonical ABI modification.

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
        case ComponentCanonOptCode::Encode_UTF8:
        case ComponentCanonOptCode::Encode_UTF16:
        case ComponentCanonOptCode::Encode_Latin1:
          spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
          spdlog::error("    incomplete canonincal options"sv);
          return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
        case ComponentCanonOptCode::Memory:
          MemInst = CompInst.getCoreMemory(Opt.getIndex());
          break;
        case ComponentCanonOptCode::Realloc:
          ReallocFunc = CompInst.getCoreFunction(Opt.getIndex());
          break;
        case ComponentCanonOptCode::PostReturn:
        case ComponentCanonOptCode::Async:
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
    case ComponentCanonOpCode::Resource__new:
    case ComponentCanonOpCode::Resource__drop:
    case ComponentCanonOpCode::Resource__rep:
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
