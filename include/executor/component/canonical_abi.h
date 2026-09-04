// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/executor/component/canonical_abi.h -----------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Component Model Canonical ABI: the helpers — alignment, element size, type
/// flattening, and the load / store / lift / lower conversions — and the
/// synthesized core functions that `canon lower`, `canon resource.*` and the
/// async built-ins install into a component's core module instances.
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
#include "runtime/instance/component/resource.h"
#include "runtime/instance/function.h"
#include "runtime/instance/memory.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace WasmEdge {
namespace Executor {

// Forward-declared: including executor/executor.h here would cycle.
class ComponentExecutor;

namespace Component {

namespace CanonicalABI {

/// Flat ABI limits.
constexpr uint32_t MaxFlatParams = 16;
constexpr uint32_t MaxFlatAsyncParams = 4;
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
  /// Borrow scope: the callee task that receives the lowered borrows.
  Runtime::Component::Task *BorrowTask = nullptr;
  /// True between components; selects the string bounds-trap vocabulary.
  bool CrossComponent = false;
  /// True when the selected memory is 64-bit, widening pointers to i64.
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

/// Alignment of a Component Model value type T under the selected memory.
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

/// Flatten a function type; `Async` and `Callback` select the async shapes.
Expect<AST::FunctionType> flattenFuncType(const Context &Cx,
                                          const AST::Component::FuncType &FT,
                                          bool IsLift, bool Async = false,
                                          bool Callback = false) noexcept;

/// Load a value of type T at Ptr; the caller pre-checks alignment and bounds.
Expect<ComponentValVariant> load(const Context &Cx, uint64_t Ptr,
                                 const ComponentValType &T) noexcept;

/// Load a defined value type — internal recursion helper.
Expect<ComponentValVariant>
loadDef(const Context &Cx, uint64_t Ptr,
        const AST::Component::DefValType &T) noexcept;

/// Store a value of type T at Ptr; string and list payloads use Cx.Realloc.
Expect<void> store(const Context &Cx, const ComponentValVariant &V,
                   const ComponentValType &T, uint64_t Ptr) noexcept;

/// Store a value into a defined value type — internal recursion helper.
Expect<void> storeDef(const Context &Cx, const ComponentValVariant &V,
                      const AST::Component::DefValType &T,
                      uint64_t Ptr) noexcept;

/// Iterator over a sequence of core wasm values backing a flat lift.
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
                std::optional<uint64_t> OutParam = std::nullopt) noexcept;

} // namespace CanonicalABI

/// Synthesized core function of `canon lower`: lift, call, lower back.
class CanonLowerHostFunc : public Runtime::HostFunctionBase {
public:
  /// FlatSig is the core signature exposed to callers and this func's type.
  CanonLowerHostFunc(ComponentExecutor *Exec, AST::FunctionType FlatSig,
                     Runtime::Instance::ComponentFunctionInstance *Callee,
                     const Runtime::Component::CanonOptions &Opts) noexcept;

  Expect<void> run(const Runtime::CallingFrame &Frame,
                   Span<const ValVariant> Args, Span<ValVariant> Rets) override;

private:
  ComponentExecutor *Exec;
  Runtime::Instance::ComponentFunctionInstance *Callee;
  Runtime::Component::CanonOptions Opts;
  // True if the signature carries a trailing out-pointer.
  bool HasOutPtr;
  // Number of leading flat argument slots holding the lowered parameters.
  uint32_t ParamSlotCount;
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

/// Immediates and options of one async canonical built-in.
struct AsyncBuiltinInfo {
  ComponentCanonOpCode Code;
  /// The canonical options declared at the built-in.
  Runtime::Component::CanonOptions Opts;
  /// The `async?` / `cancel?` immediate; read via async() or cancellable().
  bool Flag = false;
  /// stream/future ops: the element type declared at the built-in.
  std::optional<ComponentValType> Elem;
  bool IsStream = true;
  /// context.get/set slot index and slot type.
  uint32_t ContextIdx = 0;
  ValType ContextType = TypeCode::I32;
  /// task.return: declared result types.
  std::vector<ComponentValType> RetTypes;
  /// thread.new-indirect: the core table holding start functions.
  Runtime::Instance::TableInstance *Table = nullptr;

  bool async() const noexcept { return Flag; }
  bool cancellable() const noexcept { return Flag; }
};

class CanonAsyncBuiltinHostFunc : public Runtime::HostFunctionBase {
public:
  CanonAsyncBuiltinHostFunc(ComponentExecutor *ExecIn,
                            AsyncBuiltinInfo InfoIn) noexcept;

  Expect<void> run(const Runtime::CallingFrame &Frame,
                   Span<const ValVariant> Args, Span<ValVariant> Rets) override;

private:
  /// stream/future read/write rendezvous.
  Expect<void> runCopy(Span<const ValVariant> Args, Span<ValVariant> Rets);
  /// stream/future cancel-read/cancel-write.
  Expect<void> runCancelCopy(Span<const ValVariant> Args,
                             Span<ValVariant> Rets);
  /// stream/future drop-readable/drop-writable.
  Expect<void> runDropEnd(Span<const ValVariant> Args);

  ComponentExecutor *Exec;
  AsyncBuiltinInfo Info;
};

} // namespace Component
} // namespace Executor
} // namespace WasmEdge
