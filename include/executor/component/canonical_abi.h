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
#include "runtime/instance/component/component.h"
#include "runtime/instance/function.h"
#include "runtime/instance/memory.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {
namespace CanonicalABI {

/// Sync ABI limits (CanonicalABI.md L2815-2817).
constexpr uint32_t MaxFlatParams = 16;
constexpr uint32_t MaxFlatResults = 1;

/// Context bundle for canonical-ABI operations. Not every helper requires every
/// field — alignment / elem_size / flatten_* only need CompInst; load / store /
/// lift_flat / lower_flat additionally require Mem (and Realloc when allocating
/// list / string return areas).
struct CanonCtx {
  Runtime::Instance::MemoryInstance *Mem = nullptr;
  Runtime::Instance::FunctionInstance *Realloc = nullptr;
  const Runtime::Instance::ComponentInstance *CompInst = nullptr;
};

/// Discriminant byte width for a variant / enum with NumCases cases.
/// CanonicalABI.md L1951-1956 (`def discriminant_type`).
uint32_t discriminantSize(uint32_t NumCases) noexcept;

/// Alignment of a Component Model value type T (ptr_type = i32, deferred).
/// CanonicalABI.md L1904-1985.
Expect<uint32_t> alignment(const CanonCtx &Cx,
                           const ComponentValType &T) noexcept;

/// Alignment of a defined value type — internal recursion helper. Public so
/// callers that already hold a DefValType (e.g., synthesized result tuple)
/// can avoid the typeindex round-trip.
/// CanonicalABI.md L1904-1985.
Expect<uint32_t>
alignmentDef(const CanonCtx &Cx,
             const AST::Component::DefValType &T) noexcept;

/// Byte size of a Component Model value type T in linear memory.
/// CanonicalABI.md L1990-2040.
Expect<uint32_t> elemSize(const CanonCtx &Cx,
                          const ComponentValType &T) noexcept;

/// Byte size of a defined value type — internal recursion helper.
/// CanonicalABI.md L1990-2040.
Expect<uint32_t> elemSizeDef(const CanonCtx &Cx,
                             const AST::Component::DefValType &T) noexcept;

/// Result of `flatten_functype`: sequences of core wasm types representing
/// the ABI signature seen by core wasm. CanonicalABI.md L2813-2848.
struct FlatFuncType {
  std::vector<ValType> Params;
  std::vector<ValType> Results;
};

/// Flatten a Component Model value type to its core wasm representation.
/// CanonicalABI.md L2860-2877.
Expect<std::vector<ValType>>
flattenType(const CanonCtx &Cx, const ComponentValType &T) noexcept;

/// Flatten a defined value type — internal recursion helper.
/// CanonicalABI.md L2860-2877.
Expect<std::vector<ValType>>
flattenTypeDef(const CanonCtx &Cx,
               const AST::Component::DefValType &T) noexcept;

/// Flatten a component function type into its core ABI signature. Sync only
/// — async is rejected. CanonicalABI.md L2819-2832.
///
/// `IsLift = true` covers the `canon lift` direction (component-typed
/// function exposed as core wasm callee); when results exceed
/// MaxFlatResults the core function returns a single i32 return-area pointer.
/// `IsLift = false` covers `canon lower` and is currently rejected when it
/// would require indirect-return out-pointer or indirect-param paths.
Expect<FlatFuncType>
flattenFuncType(const CanonCtx &Cx, const AST::Component::FuncType &FT,
                bool IsLift) noexcept;

/// Load a Component Model value of type T from linear memory at Ptr.
/// CanonicalABI.md L2050-2289 (and L2305-2322 for own/borrow).
///
/// Cx must provide a MemoryInstance. Callers are responsible for
/// alignment and bounds pre-checks at the top level (per spec
/// `lift_flat_values` L3197-3199); load() itself relies on
/// MemoryInstance::loadValue for per-primitive bounds checking and does
/// not re-validate alignment.
Expect<ComponentValVariant> load(const CanonCtx &Cx, uint32_t Ptr,
                                 const ComponentValType &T) noexcept;

/// Load a defined value type — internal recursion helper.
Expect<ComponentValVariant> loadDef(const CanonCtx &Cx, uint32_t Ptr,
                                    const AST::Component::DefValType
                                        &T) noexcept;

/// Store a Component Model value of type T into linear memory at Ptr.
/// CanonicalABI.md L2360-2735.
///
/// String / variable-length list storage requires invoking realloc, which is
/// not yet wired through this helper — those cases return
/// ComponentNotImplInstantiate.
Expect<void> store(const CanonCtx &Cx, const ComponentValVariant &V,
                   const ComponentValType &T, uint32_t Ptr) noexcept;

/// Store a value into a defined value type — internal recursion helper.
Expect<void> storeDef(const CanonCtx &Cx, const ComponentValVariant &V,
                      const AST::Component::DefValType &T,
                      uint32_t Ptr) noexcept;

/// Iterator over a sequence of core wasm values backing a flat lift.
/// Mirrors the spec's CoreValueIter (CanonicalABI.md L2928-2948).
class FlatIter {
public:
  FlatIter(Span<const std::pair<ValVariant, ValType>> Vs) noexcept
      : Pairs(Vs), Singles{} {}
  FlatIter(Span<const ValVariant> Vs) noexcept : Pairs{}, Singles(Vs) {}

  /// Read the next core value as a ValVariant. Returns std::nullopt if the
  /// iterator is exhausted.
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

/// Lift a flat representation of a Component Model value into the rich
/// ComponentValVariant. CanonicalABI.md L2957-3084 (lift_flat).
///
/// Variant payload coercion (spec L3042-3072) is not yet implemented —
/// variants whose selected case carries a payload return
/// ComponentNotImplInstantiate.
Expect<ComponentValVariant> liftFlat(const CanonCtx &Cx, FlatIter &VI,
                                     const ComponentValType &T) noexcept;

/// Lift a defined value type from flat values — internal recursion helper.
Expect<ComponentValVariant>
liftFlatDef(const CanonCtx &Cx, FlatIter &VI,
            const AST::Component::DefValType &T) noexcept;

} // namespace CanonicalABI
} // namespace Executor
} // namespace WasmEdge
