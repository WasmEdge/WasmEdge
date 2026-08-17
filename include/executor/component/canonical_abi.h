// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/executor/component/canonical_abi.h -----------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Component Model Canonical ABI helpers (Preview 2, sync only):
///   - alignment / elem_size / flatten_type / flatten_functype
///   - load / store / lift_flat / lower_flat
///
/// Spec citations refer to
/// https://github.com/WebAssembly/component-model/blob/main/design/mvp/CanonicalABI.md
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/type.h"
#include "common/errcode.h"
#include "common/expected.h"
#include "common/types.h"
#include "runtime/component/canonopt.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/component/component.h"
#include "runtime/instance/component/function.h"
#include "runtime/instance/function.h"
#include "runtime/instance/memory.h"

#include <cstdint>
#include <functional>
#include <optional>

namespace WasmEdge {
namespace Executor {

// Forward-declared: including executor/executor.h here would cycle.
class ComponentExecutor;

namespace Component {

namespace CanonicalABI {

/// Sync ABI limits (CanonicalABI.md L2815-2817).
constexpr uint32_t MaxFlatParams = 16;
constexpr uint32_t MaxFlatResults = 1;

/// Context of one canonical-ABI operation: its options plus operation state.
struct Context : Runtime::Component::CanonOptions {
  ComponentExecutor *Exec = nullptr;
  /// Type resolver for validator-time callers; wins over Inst.
  std::function<const AST::Component::DefType *(uint32_t)> TypeResolver = {};
  /// Resource-identity resolver, set with TypeResolver; wins over Inst.
  std::function<const Runtime::Instance::Component::ResourceTypeInstance *(
      uint32_t)>
      ResourceResolver = {};
  /// Records borrows lifted here so the caller can release the lends.
  std::vector<std::pair<const Runtime::Instance::ComponentInstance *, uint32_t>>
      *LiftedBorrows = nullptr;
};

/// Resolve a component type index through TypeResolver, else Inst.
inline const AST::Component::DefType *resolveDefType(const Context &Cx,
                                                     uint32_t Idx) noexcept {
  if (Cx.TypeResolver) {
    return Cx.TypeResolver(Idx);
  }
  if (Cx.Inst != nullptr) {
    return Cx.Inst->getType(Idx);
  }
  return nullptr;
}

/// Discriminant byte width for a variant / enum with NumCases cases.
uint32_t discriminantSize(uint32_t NumCases) noexcept;

/// Alignment of a Component Model value type T in linear memory.
Expect<uint32_t> alignment(const Context &Cx,
                           const ComponentValType &T) noexcept;

/// Alignment of a defined value type, skipping the typeindex round-trip.
Expect<uint32_t> alignmentDef(const Context &Cx,
                              const AST::Component::DefValType &T) noexcept;

/// Byte size of a Component Model value type T in linear memory.
Expect<uint32_t> elemSize(const Context &Cx,
                          const ComponentValType &T) noexcept;

/// Byte size of a defined value type — internal recursion helper.
Expect<uint32_t> elemSizeDef(const Context &Cx,
                             const AST::Component::DefValType &T) noexcept;

/// Flatten a Component Model value type to its core wasm representation.
Expect<std::vector<ValType>> flattenType(const Context &Cx,
                                         const ComponentValType &T) noexcept;

/// Flatten a defined value type — internal recursion helper.
Expect<std::vector<ValType>>
flattenTypeDef(const Context &Cx, const AST::Component::DefValType &T) noexcept;

/// Flatten a function type into its core ABI signature. Sync only.
Expect<AST::FunctionType> flattenFuncType(const Context &Cx,
                                          const AST::Component::FuncType &FT,
                                          bool IsLift) noexcept;

/// Load a value of type T at Ptr; the caller pre-checks alignment and bounds.
Expect<ComponentValVariant> load(const Context &Cx, uint32_t Ptr,
                                 const ComponentValType &T) noexcept;

/// Load a defined value type — internal recursion helper.
Expect<ComponentValVariant>
loadDef(const Context &Cx, uint32_t Ptr,
        const AST::Component::DefValType &T) noexcept;

/// Store a Component Model value of type T into linear memory at Ptr.
Expect<void> store(const Context &Cx, const ComponentValVariant &V,
                   const ComponentValType &T, uint32_t Ptr) noexcept;

/// Store a value into a defined value type — internal recursion helper.
Expect<void> storeDef(const Context &Cx, const ComponentValVariant &V,
                      const AST::Component::DefValType &T,
                      uint32_t Ptr) noexcept;

/// Iterator over core wasm values backing a flat lift (spec CoreValueIter).
class FlatIter {
public:
  FlatIter(Span<const std::pair<ValVariant, ValType>> Vs) noexcept
      : Pairs(Vs), Singles{} {}
  FlatIter(Span<const ValVariant> Vs) noexcept : Pairs{}, Singles(Vs) {}

  /// Read the next core value, or std::nullopt when exhausted.
  std::optional<ValVariant> next() noexcept {
    if (!Pairs.empty()) {
      auto V = Pairs[Idx].first;
      ++Idx;
      return V;
    }
    if (Idx < Singles.size()) {
      return Singles[Idx++];
    }
    return std::nullopt;
  }
  bool done() const noexcept {
    return Idx >= (Pairs.empty() ? Singles.size() : Pairs.size());
  }
  size_t pos() const noexcept { return Idx; }

private:
  Span<const std::pair<ValVariant, ValType>> Pairs;
  Span<const ValVariant> Singles;
  size_t Idx = 0;
};

/// Lift a flat representation; mismatched variant join slots reinterpret.
Expect<ComponentValVariant> liftFlat(const Context &Cx, FlatIter &VI,
                                     const ComponentValType &T) noexcept;

/// Lift a defined value type from flat values — internal recursion helper.
Expect<ComponentValVariant>
liftFlatDef(const Context &Cx, FlatIter &VI,
            const AST::Component::DefValType &T) noexcept;

/// Lower a ComponentValVariant to flat values, the inverse of liftFlat.
Expect<std::vector<ValVariant>> lowerFlat(const Context &Cx,
                                          const ComponentValVariant &V,
                                          const ComponentValType &T) noexcept;

/// Lower a defined value type to flat values — internal recursion helper.
Expect<std::vector<ValVariant>>
lowerFlatDef(const Context &Cx, const ComponentValVariant &V,
             const AST::Component::DefValType &T) noexcept;

/// Lift a value sequence; past MaxFlat it loads a tuple through a pointer.
Expect<std::vector<ComponentValVariant>>
liftFlatValues(const Context &Cx, FlatIter &VI,
               Span<const ComponentValType> Types, uint32_t MaxFlat) noexcept;

/// Lower a value sequence; the indirect case stores into OutParam.
Expect<std::vector<ValVariant>>
lowerFlatValues(const Context &Cx, Span<const ComponentValVariant> Values,
                Span<const ComponentValType> Types, uint32_t MaxFlat,
                std::optional<uint32_t> OutParam = std::nullopt) noexcept;

} // namespace CanonicalABI

/// Synthesized core function of `canon lower`: lift, call, lower back.
class CanonLowerHostFunc : public Runtime::HostFunctionBase {
public:
  /// FlatSig is the core signature exposed to callers and this func's type.
  CanonLowerHostFunc(ComponentExecutor *Exec, AST::FunctionType FlatSig,
                     Runtime::Instance::Component::FunctionInstance *Callee,
                     const Runtime::Component::CanonOptions &Opts) noexcept;

  Expect<void> run(const Runtime::CallingFrame &Frame,
                   Span<const ValVariant> Args, Span<ValVariant> Rets) override;

private:
  ComponentExecutor *Exec;
  Runtime::Instance::Component::FunctionInstance *Callee;
  Runtime::Component::CanonOptions Opts;
  // True if the signature carries a trailing out-pointer.
  bool HasOutPtr;
};

/// canon resource.new $rt : [rep:i32] -> [i32]
class CanonResourceNewHostFunc : public Runtime::HostFunctionBase {
public:
  CanonResourceNewHostFunc(
      const Runtime::Instance::ComponentInstance *Inst,
      const Runtime::Instance::Component::ResourceTypeInstance *RT) noexcept;

  Expect<void> run(const Runtime::CallingFrame &Frame,
                   Span<const ValVariant> Args, Span<ValVariant> Rets) override;

private:
  const Runtime::Instance::ComponentInstance *Inst;
  const Runtime::Instance::Component::ResourceTypeInstance *RT;
};

/// canon resource.rep $rt : [i32] -> [rep:i32]
class CanonResourceRepHostFunc : public Runtime::HostFunctionBase {
public:
  CanonResourceRepHostFunc(
      const Runtime::Instance::ComponentInstance *Inst,
      const Runtime::Instance::Component::ResourceTypeInstance *RT) noexcept;

  Expect<void> run(const Runtime::CallingFrame &Frame,
                   Span<const ValVariant> Args, Span<ValVariant> Rets) override;

private:
  const Runtime::Instance::ComponentInstance *Inst;
  const Runtime::Instance::Component::ResourceTypeInstance *RT;
};

/// canon resource.drop $rt : [i32] -> []
class CanonResourceDropHostFunc : public Runtime::HostFunctionBase {
public:
  CanonResourceDropHostFunc(
      ComponentExecutor *Exec, const Runtime::Instance::ComponentInstance *Inst,
      const Runtime::Instance::Component::ResourceTypeInstance *RT) noexcept;

  Expect<void> run(const Runtime::CallingFrame &Frame,
                   Span<const ValVariant> Args, Span<ValVariant> Rets) override;

private:
  ComponentExecutor *Exec;
  const Runtime::Instance::ComponentInstance *Inst;
  const Runtime::Instance::Component::ResourceTypeInstance *RT;
};

} // namespace Component
} // namespace Executor
} // namespace WasmEdge
