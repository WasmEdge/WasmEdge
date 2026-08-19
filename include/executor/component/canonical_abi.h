// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/executor/component/canonical_abi.h -----------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Component Model Canonical ABI helpers: alignment, element size, type
/// flattening, and the load / store / lift / lower conversions.
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
#include <functional>
#include <optional>

namespace WasmEdge {
namespace Executor {

namespace Component {

// Forward-declared: store / lowerFlat reach the guest `realloc` through the
// core executor, and including executor/executor.h here would cycle.
class Executor;

class Task;

namespace CanonicalABI {

/// Flat ABI limits.
constexpr uint32_t MaxFlatParams = 16;
constexpr uint32_t MaxFlatAsyncParams = 4;
constexpr uint32_t MaxFlatResults = 1;

/// Context bundle for canonical-ABI operations. Each helper reads only what it
/// needs: a type resolver always, Mem for memory access, Realloc to allocate.
struct CanonCtx {
  Executor *Exec = nullptr;
  Runtime::Instance::MemoryInstance *Mem = nullptr;
  Runtime::Instance::FunctionInstance *Realloc = nullptr;
  const Runtime::Instance::ComponentInstance *CompInst = nullptr;
  /// Resolves type indices for validator-time callers, which hold a
  /// Validator::Component::Context but no instance. Wins over CompInst.
  std::function<const AST::Component::DefType *(uint32_t)> TypeResolver;
  /// Resolves the resource identity of own/borrow type indices. Set with
  /// TypeResolver, and it takes precedence over CompInst.
  std::function<const Runtime::Instance::Component::ResourceTypeRT *(uint32_t)>
      ResourceResolver;
  /// When set, borrows lifted through this context are recorded so the
  /// caller can release the lends after the call returns.
  std::vector<std::pair<const Runtime::Instance::ComponentInstance *, uint32_t>>
      *LiftedBorrows = nullptr;
  /// Guest string encoding of this canon function; defaults to UTF-8.
  StringEncoding Enc = StringEncoding::UTF8;
  /// Borrow scope: the callee task that receives the borrows lowered here,
  /// for its borrow accounting.
  Task *BorrowTask = nullptr;
  /// True between two component instances, false at the host boundary. It
  /// selects the diagnostic vocabulary for the string bounds traps.
  bool CrossComponent = false;
  /// True when the selected memory is 64-bit, which widens every pointer and
  /// length this ABI passes from i32 to i64.
  bool memory64() const noexcept {
    return Mem != nullptr && Mem->getMemoryType().getLimit().is64();
  }
  /// The core type of a pointer or length under the selected memory.
  ValType ptrType() const noexcept {
    return memory64() ? ValType(TypeCode::I64) : ValType(TypeCode::I32);
  }
  /// Byte width and alignment of a pointer or length.
  uint32_t ptrSize() const noexcept { return memory64() ? 8u : 4u; }
};

/// Resolve a component type index. Returns the DefType pointer using
/// TypeResolver when present, otherwise falling back to CompInst.
inline const AST::Component::DefType *resolveDefType(const CanonCtx &Cx,
                                                     uint32_t Idx) noexcept {
  if (Cx.TypeResolver) {
    return Cx.TypeResolver(Idx);
  }
  if (Cx.CompInst != nullptr) {
    return Cx.CompInst->getType(Idx);
  }
  return nullptr;
}

/// Discriminant byte width for a variant / enum with NumCases cases.
uint32_t discriminantSize(uint32_t NumCases) noexcept;

/// Alignment of a Component Model value type T under the selected memory.
Expect<uint32_t> alignment(const CanonCtx &Cx,
                           const ComponentValType &T) noexcept;

/// Alignment of a defined value type — recursion helper, public so callers
/// already holding a DefValType skip the typeindex round-trip.
Expect<uint32_t> alignmentDef(const CanonCtx &Cx,
                              const AST::Component::DefValType &T) noexcept;

/// Byte size of a Component Model value type T in linear memory.
Expect<uint32_t> elemSize(const CanonCtx &Cx,
                          const ComponentValType &T) noexcept;

/// Byte size of a defined value type — internal recursion helper.
Expect<uint32_t> elemSizeDef(const CanonCtx &Cx,
                             const AST::Component::DefValType &T) noexcept;

/// The core wasm types of a flattened function signature.
struct FlatFuncType {
  std::vector<ValType> Params;
  std::vector<ValType> Results;
};

/// Flatten a Component Model value type to its core wasm representation.
Expect<std::vector<ValType>> flattenType(const CanonCtx &Cx,
                                         const ComponentValType &T) noexcept;

/// Flatten a defined value type — internal recursion helper.
Expect<std::vector<ValType>>
flattenTypeDef(const CanonCtx &Cx,
               const AST::Component::DefValType &T) noexcept;

/// Flatten a function type into its core ABI signature. `Async` and
/// `Callback` are the canon options selecting the async shapes.
Expect<FlatFuncType> flattenFuncType(const CanonCtx &Cx,
                                     const AST::Component::FuncType &FT,
                                     bool IsLift, bool Async = false,
                                     bool Callback = false) noexcept;

/// True iff `T` transitively contains a `list` or `string`. The caller passes
/// a guard set to bound type-index cycles.
bool containsListOrString(const CanonCtx &Cx,
                          const ComponentValType &T) noexcept;

/// Load a value of type T from linear memory at Ptr. The caller pre-checks
/// alignment and top-level bounds.
Expect<ComponentValVariant> load(const CanonCtx &Cx, uint64_t Ptr,
                                 const ComponentValType &T) noexcept;

/// Load a defined value type — internal recursion helper.
Expect<ComponentValVariant>
loadDef(const CanonCtx &Cx, uint64_t Ptr,
        const AST::Component::DefValType &T) noexcept;

/// Store a Component Model value of type T into linear memory at Ptr; string
/// and variable-length list payloads allocate via `Cx.Realloc`.
Expect<void> store(const CanonCtx &Cx, const ComponentValVariant &V,
                   const ComponentValType &T, uint64_t Ptr) noexcept;

/// Store a value into a defined value type — internal recursion helper.
Expect<void> storeDef(const CanonCtx &Cx, const ComponentValVariant &V,
                      const AST::Component::DefValType &T,
                      uint64_t Ptr) noexcept;

/// Iterator over a sequence of core wasm values backing a flat lift.
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

private:
  Span<const std::pair<ValVariant, ValType>> Pairs;
  Span<const ValVariant> Singles;
  size_t Idx = 0;
};

/// Lift a flat representation into the rich ComponentValVariant. Mismatched
/// variant join slots reinterpret.
Expect<ComponentValVariant> liftFlat(const CanonCtx &Cx, FlatIter &VI,
                                     const ComponentValType &T) noexcept;

/// Lift a defined value type from flat values — internal recursion helper.
Expect<ComponentValVariant>
liftFlatDef(const CanonCtx &Cx, FlatIter &VI,
            const AST::Component::DefValType &T) noexcept;

/// Lower a ComponentValVariant to flat values, the inverse of `liftFlat`.
/// String / list payloads need `Cx.Realloc`.
Expect<std::vector<ValVariant>> lowerFlat(const CanonCtx &Cx,
                                          const ComponentValVariant &V,
                                          const ComponentValType &T) noexcept;

/// Lower a defined value type to flat values — internal recursion helper.
Expect<std::vector<ValVariant>>
lowerFlatDef(const CanonCtx &Cx, const ComponentValVariant &V,
             const AST::Component::DefValType &T) noexcept;

/// Lift a value sequence. Past MaxFlat, reads one pointer and loads the
/// synthesized tuple from memory instead of the flat values.
Expect<std::vector<ComponentValVariant>>
liftFlatValues(const CanonCtx &Cx, FlatIter &VI,
               Span<const ComponentValType> Types, uint32_t MaxFlat) noexcept;

/// Lower a value sequence. The indirect case stores into OutParam and
/// returns nothing, else reallocs a buffer and returns its i32.
Expect<std::vector<ValVariant>>
lowerFlatValues(const CanonCtx &Cx, Span<const ComponentValVariant> Values,
                Span<const ComponentValType> Types, uint32_t MaxFlat,
                std::optional<uint64_t> OutParam = std::nullopt) noexcept;

} // namespace CanonicalABI
} // namespace Component
} // namespace Executor
} // namespace WasmEdge
