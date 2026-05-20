// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/component/canonical_abi.h"
#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <cstdint>
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
      // realloc(old_ptr=0, old_size=0, alignment=1, new_size=StrSize)
      // Alignment = 1 for UTF-8 strings (CanonicalABI spec L2432).
      // TODO: When UTF-16 or Latin1+UTF16 encoding support is added,
      // alignment should be 2 (CanonicalABI spec L2438, L2446).
      std::vector<ValVariant> ReallocArgs{ValVariant(0), ValVariant(0),
                                          ValVariant(1), ValVariant(StrSize)};
      std::vector<ValType> ReallocTypes =
          RFuncInst->getFuncType().getParamTypes();
      auto AllocRes = invoke(RFuncInst, ReallocArgs, ReallocTypes);
      // realloc's signature is not validated yet (GAP-C-5b); it must return
      // one i32 pointer, so guard against a missing or empty result.
      if (!AllocRes || AllocRes->empty()) {
        CoreVals.push_back(0);
        CoreVals.push_back(0);
        break;
      }
      ValVariant PtrInMem = (*AllocRes)[0].first;
      if (!MemInst->setBytes(std::vector<Byte>{Str.begin(), Str.end()},
                             PtrInMem.get<uint32_t>(), 0,
                             static_cast<uint32_t>(Str.size()))) {
        CoreVals.push_back(0);
        CoreVals.push_back(0);
        break;
      }
      CoreVals.push_back(PtrInMem);
      CoreVals.push_back(StrSize);
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
    Runtime::Instance::MemoryInstance *MemInst,
    const Runtime::Instance::ComponentInstance *CompInst) {
  CanonicalABI::CanonCtx Cx{MemInst, nullptr, CompInst};

  // Compute total flat count over the result types. CanonicalABI.md L3193.
  uint32_t FlatCount = 0;
  for (const auto &Type : ValTypes) {
    EXPECTED_TRY(auto Sub, CanonicalABI::flattenType(Cx, Type));
    FlatCount += static_cast<uint32_t>(Sub.size());
  }

  std::vector<std::pair<ComponentValVariant, ComponentValType>> Vals;
  Vals.reserve(ValTypes.size());

  if (FlatCount > CanonicalABI::MaxFlatResults) {
    // Indirect-return path. CanonicalABI.md L3194-3200:
    //   ptr = vi.next(ptr_type)
    //   trap_if(ptr != align_to(ptr, alignment(tuple_type, ptr_type)))
    //   trap_if(ptr + elem_size(tuple_type, ptr_type) > len(memory))
    //   return list(load(cx, ptr, tuple_type).values())
    if (CoreVals.size() != 1) {
      spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
      spdlog::error(
          "    indirect-return: expected 1 core return value, got {}"sv,
          CoreVals.size());
      return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
    }
    assuming(MemInst != nullptr);
    const uint32_t Ptr = CoreVals[0].first.get<uint32_t>();

    // Build a synthetic tuple type covering all result types so we can use
    // alignment / elem_size / load uniformly. Spec L3197.
    AST::Component::TupleTy ResultsTuple;
    for (const auto &VT : ValTypes) {
      ResultsTuple.Types.push_back(VT);
    }
    AST::Component::DefValType ResultsTupleDef;
    ResultsTupleDef.setTuple(std::move(ResultsTuple));

    EXPECTED_TRY(auto Align,
                 CanonicalABI::alignmentDef(Cx, ResultsTupleDef));
    EXPECTED_TRY(auto Sz, CanonicalABI::elemSizeDef(Cx, ResultsTupleDef));
    if (Ptr != alignTo(Ptr, Align)) {
      spdlog::error(ErrCode::Value::MemoryOutOfBounds);
      spdlog::error(
          "    indirect-return pointer 0x{:x} not aligned to {}"sv, Ptr,
          Align);
      return Unexpect(ErrCode::Value::MemoryOutOfBounds);
    }
    if (!MemInst->checkAccessBound(Ptr, Sz)) {
      spdlog::error(ErrCode::Value::MemoryOutOfBounds);
      spdlog::error(
          "    indirect-return area out of bounds (ptr=0x{:x} size={})"sv,
          Ptr, Sz);
      return Unexpect(ErrCode::Value::MemoryOutOfBounds);
    }
    EXPECTED_TRY(auto Loaded,
                 CanonicalABI::loadDef(Cx, Ptr, ResultsTupleDef));
    auto &Tu = std::get<TupleVal>(
        std::get<std::shared_ptr<ValComp>>(Loaded)->V);
    assuming(Tu.Values.size() == ValTypes.size());
    for (size_t Idx = 0; Idx < ValTypes.size(); ++Idx) {
      Vals.emplace_back(std::move(Tu.Values[Idx]), ValTypes[Idx]);
    }
    return Vals;
  }

  // Direct path. Preserve the legacy "primitives wrapped in ValVariant"
  // calling convention so existing host-side consumers (driver, host-function
  // marshaling) are unaffected. Aggregate result types in the direct path
  // (flat count ≤ 1 record/tuple/etc.) are not yet handled.
  uint32_t I = 0;
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
    case ComponentTypeCode::TypeIndex: {
      spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
      spdlog::error(
          "    direct-path lift of aggregate type not implemented"sv);
      return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
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
      // Pre-flight the ABI signature: this surfaces gated / out-of-scope
      // shapes (async, indirect-params, lower-side indirect, etc.) at
      // instantiation time rather than at call time. Sync-lift indirect
      // results are explicitly supported by B and reduce to results=[i32].
      {
        CanonicalABI::CanonCtx PrefCx{nullptr, nullptr, &CompInst};
        EXPECTED_TRY(CanonicalABI::flattenFuncType(PrefCx, DType->getFuncType(),
                                                   /*IsLift=*/true));
      }
      auto *FuncInst = CompInst.getCoreFunction(Canon.getIndex());
      CompInst.addFunction(
          std::make_unique<Runtime::Instance::Component::FunctionInstance>(
              DType->getFuncType(), FuncInst, MemInst, ReallocFunc,
              &CompInst));
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
