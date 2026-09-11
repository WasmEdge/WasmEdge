// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/executor/component/canonical_abi.h -----------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Component Model Canonical ABI: the lift/lower context with its layout,
/// load / store and lift / lower operations, and the synthesized core
/// functions that `canon lower`, `canon resource.*` and the async built-ins
/// install into a component's core module instances.
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/type.h"
#include "common/component_valtype.h"
#include "common/component_variant.h"
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
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Executor {

// Forward-declared: including executor/executor.h here would cycle.
class ComponentExecutor;

namespace Component {

/// The lift/lower context of the canonical ABI, CanonicalABI.md's
/// LiftLowerContext: the options one lift or lower runs under, the instance
/// its type indices resolve in, the borrow scope, and the operations of the
/// ABI over them.
class LiftLowerContext {
public:
  /// Flat ABI limits.
  static constexpr uint32_t MaxFlatParams = 16;
  static constexpr uint32_t MaxFlatAsyncParams = 4;
  static constexpr uint32_t MaxFlatResults = 1;
  /// The upper bound on string and list byte lengths.
  static constexpr uint32_t MaxCanonByteLength = (1U << 28) - 1U;

  LiftLowerContext() noexcept = default;
  /// The context of a lift or lower under Opts run by CompExec, whose type
  /// indices belong to TypeInstance when that is not the instance of Opts.
  LiftLowerContext(const Runtime::Component::CanonOptions &Opts,
                   ComponentExecutor *CompExec = nullptr,
                   const Runtime::Instance::ComponentInstance *TypeInstance =
                       nullptr) noexcept
      : Inst(Opts.Inst), Mem(Opts.Mem), Realloc(Opts.Realloc), Enc(Opts.Enc),
        Exec(CompExec), TypeInst(TypeInstance) {}

  /// \name The lift/lower options.
  /// @{
  const Runtime::Instance::ComponentInstance *getInstance() const noexcept {
    return Inst;
  }
  Runtime::Instance::MemoryInstance *getMemory() const noexcept { return Mem; }
  Runtime::Instance::FunctionInstance *getRealloc() const noexcept {
    return Realloc;
  }
  Runtime::Component::StringEncoding getEncoding() const noexcept {
    return Enc;
  }
  /// @}

  /// \name The state of the operation.
  /// @{
  ComponentExecutor *getExecutor() const noexcept { return Exec; }
  /// The instance the type indices resolve in: the type instance, else the
  /// instance of the options.
  const Runtime::Instance::ComponentInstance *getTypeInstance() const noexcept {
    return TypeInst != nullptr ? TypeInst : Inst;
  }
  void
  setTypeInstance(const Runtime::Instance::ComponentInstance *TI) noexcept {
    TypeInst = TI;
  }
  /// The borrows lifted here, for the caller to release their lends.
  std::vector<
      std::pair<const Runtime::Instance::ComponentInstance *, uint32_t>> *
  getLiftedBorrows() const noexcept {
    return LiftedBorrows;
  }
  void setLiftedBorrows(
      std::vector<std::pair<const Runtime::Instance::ComponentInstance *,
                            uint32_t>> *L) noexcept {
    LiftedBorrows = L;
  }
  /// The borrow scope: the callee task that receives the lowered borrows.
  Runtime::Component::Task *getBorrowTask() const noexcept {
    return BorrowTask;
  }
  void setBorrowTask(Runtime::Component::Task *T) noexcept { BorrowTask = T; }
  /// True between components; selects the string bounds-trap vocabulary.
  bool isCrossComponent() const noexcept { return CrossComponent; }
  void setCrossComponent(bool B) noexcept { CrossComponent = B; }
  /// @}

  /// \name Type lookups in the type instance.
  /// @{
  /// The definition behind a component type index, if any.
  const AST::Component::DefType *getType(uint32_t Idx) const noexcept {
    const auto *TI = getTypeInstance();
    if (TI == nullptr) {
      return nullptr;
    }
    const auto Def = TI->getType(Idx);
    return Def ? *Def : nullptr;
  }
  /// The resource identity behind a handle type index, if any.
  const Runtime::Instance::Component::ResourceTypeInstance *
  getTypeResource(uint32_t Idx) const noexcept {
    const auto *TI = getTypeInstance();
    if (TI == nullptr) {
      return nullptr;
    }
    const auto Resource = TI->getTypeResource(Idx);
    return Resource ? *Resource : nullptr;
  }
  /// @}

  /// \name The address width of the selected memory.
  /// @{
  /// True when the selected memory is 64-bit, widening pointers to i64.
  bool isMemory64() const noexcept {
    return Mem != nullptr && Mem->getMemoryType().getLimit().is64();
  }
  /// The core type of a pointer or length under the selected memory.
  ValType getPtrType() const noexcept {
    return isMemory64() ? ValType(TypeCode::I64) : ValType(TypeCode::I32);
  }
  /// Byte width and alignment of a pointer or length.
  uint32_t getPtrSize() const noexcept { return isMemory64() ? 8u : 4u; }
  /// Read or write a pointer or length in memory, at its address width.
  Expect<uint64_t> loadPtr(uint64_t Ptr) const noexcept;
  Expect<void> storePtr(uint64_t V, uint64_t Ptr) const noexcept;
  /// A pointer or length lifted from, or lowered into, a flat slot.
  uint64_t liftPtr(const ValVariant &Slot) const noexcept;
  ValVariant lowerPtr(uint64_t Ptr) const noexcept;
  /// @}

  /// \name Type layout. Each operation: a primitive leaf, the defined value
  /// type recursion, and the component value type resolving its index here.
  /// @{
  /// Discriminant byte width for a variant / enum with NumCases cases.
  static uint32_t discriminantSize(uint32_t NumCases) noexcept;
  /// The element type a map's entries are laid out as: (tuple k v).
  static AST::Component::DefValType
  mapEntryType(const AST::Component::MapTy &M) noexcept;
  /// Alignment of a value type under the selected memory.
  Expect<uint32_t> alignment(PrimValType T) const noexcept;
  Expect<uint32_t> alignment(const ComponentValType &T) const noexcept;
  Expect<uint32_t>
  alignment(const AST::Component::DefValType &T) const noexcept;
  /// Byte size of a value type in linear memory.
  Expect<uint32_t> elemSize(PrimValType T) const noexcept;
  Expect<uint32_t> elemSize(const ComponentValType &T) const noexcept;
  Expect<uint32_t> elemSize(const AST::Component::DefValType &T) const noexcept;
  /// The core types a value type flattens to.
  Expect<std::vector<ValType>> flattenType(PrimValType T) const noexcept;
  Expect<std::vector<ValType>>
  flattenType(const ComponentValType &T) const noexcept;
  Expect<std::vector<ValType>>
  flattenType(const AST::Component::DefValType &T) const noexcept;
  /// Flatten a function type; `Async` and `Callback` select the async
  /// shapes.
  Expect<AST::FunctionType>
  flattenFuncType(const AST::Component::FuncType &FnType, bool IsLift,
                  bool Async = false, bool Callback = false) const noexcept;
  /// Structural equality of two value types, each read in the type index
  /// space of its instance; handle types compare by resource identity.
  static bool valTypeEq(const Runtime::Instance::ComponentInstance *AInst,
                        const ComponentValType &A,
                        const Runtime::Instance::ComponentInstance *BInst,
                        const ComponentValType &B, uint32_t Depth = 0) noexcept;
  static bool valTypeEq(const Runtime::Instance::ComponentInstance *AInst,
                        const AST::Component::DefValType &A,
                        const Runtime::Instance::ComponentInstance *BInst,
                        const AST::Component::DefValType &B,
                        uint32_t Depth = 0) noexcept;
  /// The optional payload types of two streams or futures: both absent, or
  /// both present and equal.
  static bool valTypeEq(const Runtime::Instance::ComponentInstance *AInst,
                        const std::optional<ComponentValType> &A,
                        const Runtime::Instance::ComponentInstance *BInst,
                        const std::optional<ComponentValType> &B) noexcept;
  /// @}

  /// \name Values in memory.
  /// @{
  /// Load a value of type T at Ptr; the caller pre-checks alignment and
  /// bounds.
  Expect<ComponentValVariant> load(uint64_t Ptr, PrimValType T) const noexcept;
  Expect<ComponentValVariant> load(uint64_t Ptr,
                                   const ComponentValType &T) const noexcept;
  Expect<ComponentValVariant>
  load(uint64_t Ptr, const AST::Component::DefValType &T) const noexcept;
  /// Store a value of type T at Ptr; string and list payloads go through
  /// realloc. V holds the alternatives of T, as checkValue establishes.
  Expect<void> store(const ComponentValVariant &V, PrimValType T,
                     uint64_t Ptr) const noexcept;
  Expect<void> store(const ComponentValVariant &V, const ComponentValType &T,
                     uint64_t Ptr) const noexcept;
  Expect<void> store(const ComponentValVariant &V,
                     const AST::Component::DefValType &T,
                     uint64_t Ptr) const noexcept;
  /// @}

  /// \name Values in flat slots. A flat slot is the ValVariant arm of its
  /// core type; a component value crosses into and out of it as the bit
  /// pattern, which is also how a variant case fits the joined slot of its
  /// siblings.
  /// @{
  /// The bit pattern of the flat slot V of core type T; i32 and f32 patterns
  /// are zero-extended.
  static uint64_t slotBits(const ValVariant &V, ValType T) noexcept;
  /// The flat slot of core type T holding Bits, truncated to the width of T.
  static ValVariant slotFromBits(ValType T, uint64_t Bits) noexcept;
  /// Lift one value of type T from its flat slots at Flat[Pos..], advancing
  /// Pos past them. The mirror of load.
  Expect<ComponentValVariant> lift(Span<const ValVariant> Flat, size_t &Pos,
                                   PrimValType T) const noexcept;
  Expect<ComponentValVariant> lift(Span<const ValVariant> Flat, size_t &Pos,
                                   const ComponentValType &T) const noexcept;
  Expect<ComponentValVariant>
  lift(Span<const ValVariant> Flat, size_t &Pos,
       const AST::Component::DefValType &T) const noexcept;
  /// Lower one value of type T, appending its flat slots to Flat. The
  /// mirror of store. V holds the alternatives of T, as checkValue
  /// establishes.
  Expect<void> lower(const ComponentValVariant &V, PrimValType T,
                     std::vector<ValVariant> &Flat) const noexcept;
  Expect<void> lower(const ComponentValVariant &V, const ComponentValType &T,
                     std::vector<ValVariant> &Flat) const noexcept;
  Expect<void> lower(const ComponentValVariant &V,
                     const AST::Component::DefValType &T,
                     std::vector<ValVariant> &Flat) const noexcept;
  /// lift_flat_values: the value sequence of Types from its flat slots,
  /// which past MaxFlat hold one pointer to a tuple of the values in memory.
  Expect<std::vector<ComponentValVariant>>
  liftValues(Span<const ValVariant> Flat, Span<const ComponentValType> Types,
             uint32_t MaxFlat) const noexcept;
  /// lower_flat_values: the flat slots of a value sequence of Types, which
  /// past MaxFlat go into a tuple in memory at OutParam, or at a fresh
  /// allocation whose pointer is then the one slot.
  Expect<std::vector<ValVariant>>
  lowerValues(Span<const ComponentValVariant> Values,
              Span<const ComponentValType> Types, uint32_t MaxFlat,
              std::optional<uint64_t> OutParam = std::nullopt) const noexcept;
  /// @}

  /// Check that a host-side value holds the alternatives type T lowers
  /// from, recursively; the lowering and storing paths read a value
  /// unchecked after it. Fails with FuncSigMismatch.
  Expect<void> checkValue(const ComponentValVariant &V,
                          PrimValType T) const noexcept;
  Expect<void> checkValue(const ComponentValVariant &V,
                          const ComponentValType &T) const noexcept;
  Expect<void> checkValue(const ComponentValVariant &V,
                          const AST::Component::DefValType &T) const noexcept;

  /// \name Value hygiene and traps.
  /// @{
  /// Every NaN crossing the ABI collapses to the canonical quiet NaN.
  static float canonicalizeNaN32(float F) noexcept;
  static double canonicalizeNaN64(double F) noexcept;
  /// Trap on a code point above 0x10FFFF or a surrogate from the guest.
  static Expect<void> validateUSV(uint32_t Value) noexcept;
  /// A host value's char is the host's responsibility.
  static void assumeValidUSV(uint32_t Value) noexcept;
  /// The case index of a variant or enum value, resolving a label.
  static uint32_t
  resolveVariantCase(const VariantVal &Val,
                     const AST::Component::VariantTy &Ty) noexcept;
  static uint32_t resolveEnumCase(const EnumVal &Val,
                                  const AST::Component::EnumTy &Ty) noexcept;
  /// The bit set of a flags value, resolving set labels.
  static uint64_t packFlags(const FlagsVal &Val,
                            const AST::Component::FlagsTy &Ty) noexcept;
  /// Trap with an execution-band code on malformed guest data.
  static Expect<void>
  trapDataInvalid(std::string_view Detail,
                  ErrCode::Value Code = ErrCode::Value::ComponentTrap) noexcept;
  /// @}

private:
  /// \name Layout constants and the memory trap.
  /// @{
  /// Discriminant widths: 1..256 cases take 1 byte, 257..65536 2 bytes.
  static constexpr uint32_t DiscriminantU8Cases = 256;
  static constexpr uint32_t DiscriminantU16Cases = 65536;
  /// The UTF-16 marker of a tagged string length: the high bit.
  static constexpr uint32_t Utf16LengthTag = 0x80000000U;
  /// The canonical quiet NaN bit patterns.
  static constexpr uint32_t CanonicalF32NaNBits = 0x7fc00000U;
  static constexpr uint64_t CanonicalF64NaNBits = 0x7ff8000000000000ULL;
  /// Trap on an out-of-bounds region of the selected memory.
  Expect<void> trapMemoryOOB(uint64_t Ptr, uint64_t Len) const noexcept;
  /// @}

  /// \name Representation of handles, transmit ends, error contexts, strings,
  /// and realloc, in the selected memory and the instance's tables.
  /// @{
  Expect<uint64_t> liftHandle(uint32_t TypeIdx, uint32_t Idx,
                              bool Own) const noexcept;
  uint32_t lowerHandle(uint32_t TypeIdx, uint64_t Rep, bool Own) const noexcept;
  Expect<std::shared_ptr<void>> liftTransmitEnd(bool IsStream,
                                                uint32_t Idx) const noexcept;
  Expect<uint32_t>
  lowerTransmitEnd(bool IsStream,
                   const std::shared_ptr<void> &SharedV) const noexcept;
  Expect<ComponentValVariant> liftErrorContext(uint32_t Idx) const noexcept;
  Expect<uint32_t>
  lowerErrorContext(const ComponentValVariant &V) const noexcept;
  Expect<uint64_t> callRealloc(uint64_t OldPtr, uint64_t OldSize,
                               uint32_t Align, uint64_t NewSize) const noexcept;
  Expect<std::string> decodeString(uint64_t Begin,
                                   uint64_t TaggedCodeUnits) const noexcept;
  Expect<std::pair<uint64_t, uint64_t>>
  encodeString(const std::string &S) const noexcept;
  /// @}

  /// \name Layout leaves.
  /// @{
  /// The core type of a flat slot of a primitive: one slot, or for string
  /// the pointer type of the (ptr, len) pair.
  Expect<ValType> primSlotType(PrimValType T) const noexcept;
  /// Maximum payload alignment across a variant's cases, 1 if none.
  Expect<uint32_t> maxCaseAlignment(
      const std::vector<std::pair<std::string, std::optional<ComponentValType>>>
          &Cases) const noexcept;
  /// The flat slots a type list occupies.
  Expect<uint32_t>
  totalFlatCount(Span<const ComponentValType> Types) const noexcept;
  /// @}

  /// \name Index resolution and list payloads.
  /// @{
  /// The value type definition at Idx; an index of any other kind of type
  /// is an invalid type reference.
  Expect<const AST::Component::DefValType *>
  getDefValType(uint32_t Idx) const noexcept;
  /// The context of the definition at Idx: this one, unless the type was
  /// aliased from another instance, whose index space its nested types use.
  LiftLowerContext contextOf(uint32_t Idx) const noexcept {
    LiftLowerContext Out = *this;
    if (const auto *TI = getTypeInstance(); TI != nullptr) {
      if (const auto Owner = TI->getTypeOwner(Idx);
          Owner && *Owner != nullptr && *Owner != TI) {
        Out.TypeInst = *Owner;
      }
    }
    return Out;
  }
  /// Load the list payload at (Begin, Length) of elements ElemT.
  Expect<ComponentValVariant> loadList(uint64_t Begin, uint64_t Length,
                                       PrimValType ElemT) const noexcept;
  Expect<ComponentValVariant>
  loadList(uint64_t Begin, uint64_t Length,
           const ComponentValType &ElemT) const noexcept;
  Expect<ComponentValVariant>
  loadList(uint64_t Begin, uint64_t Length,
           const AST::Component::DefValType &ElemT) const noexcept;
  /// Store the elements of Lv as a fresh list payload; its (ptr, len).
  Expect<std::pair<uint64_t, uint64_t>>
  storeList(const ListVal &Lv, PrimValType ElemT) const noexcept;
  Expect<std::pair<uint64_t, uint64_t>>
  storeList(const ListVal &Lv, const ComponentValType &ElemT) const noexcept;
  Expect<std::pair<uint64_t, uint64_t>>
  storeList(const ListVal &Lv,
            const AST::Component::DefValType &ElemT) const noexcept;
  /// @}

  /// \name Data of the lift/lower context.
  /// @{
  const Runtime::Instance::ComponentInstance *Inst = nullptr;
  Runtime::Instance::MemoryInstance *Mem = nullptr;
  Runtime::Instance::FunctionInstance *Realloc = nullptr;
  Runtime::Component::StringEncoding Enc =
      Runtime::Component::StringEncoding::UTF8;
  ComponentExecutor *Exec = nullptr;
  const Runtime::Instance::ComponentInstance *TypeInst = nullptr;
  std::vector<std::pair<const Runtime::Instance::ComponentInstance *, uint32_t>>
      *LiftedBorrows = nullptr;
  Runtime::Component::Task *BorrowTask = nullptr;
  bool CrossComponent = false;
  /// @}
};

/// Synthesized core function of `canon lower`: lift, call, lower back.
class CanonLowerHostFunc : public Runtime::HostFunctionBase {
public:
  /// FlatSig is the core signature exposed to callers and this func's type.
  CanonLowerHostFunc(ComponentExecutor *CompExec, AST::FunctionType FlatSig,
                     Runtime::Instance::ComponentFunctionInstance *CalleeFunc,
                     const Runtime::Component::CanonOptions &Options) noexcept;

  Expect<void> run(const Runtime::CallingFrame &Frame,
                   Span<const ValVariant> Args, Span<ValVariant> Rets) override;

private:
  /// The caller-side context: the callee's type indices over the caller's
  /// handle tables.
  LiftLowerContext makeCallerContext() const noexcept;

  ComponentExecutor *Exec;
  Runtime::Instance::ComponentFunctionInstance *Callee;
  Runtime::Component::CanonOptions Opts;
  /// The declared parameter and result types of the callee.
  std::vector<ComponentValType> ParamTypes;
  std::vector<ComponentValType> ResultTypes;
  /// The flat limits of this lowering direction and mode.
  uint32_t MaxFlatParams;
  uint32_t MaxFlatResults;
  /// True if the signature carries a trailing out-pointer.
  bool HasOutPtr;
  /// Number of leading flat argument slots holding the lowered parameters.
  uint32_t ParamSlotCount;
};

/// canon resource.new $rt : [rep:i32] -> [i32]
class CanonResourceNewHostFunc : public Runtime::HostFunctionBase {
public:
  CanonResourceNewHostFunc(
      const Runtime::Instance::ComponentInstance *CompInst,
      const Runtime::Instance::Component::ResourceTypeInstance
          *ResType) noexcept;

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
      const Runtime::Instance::ComponentInstance *CompInst,
      const Runtime::Instance::Component::ResourceTypeInstance
          *ResType) noexcept;

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
      ComponentExecutor *CompExec,
      const Runtime::Instance::ComponentInstance *CompInst,
      const Runtime::Instance::Component::ResourceTypeInstance
          *ResType) noexcept;

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
  /// The `async?` / `cancel?` immediate; read via isAsync() or
  /// isCancellable().
  bool Flag = false;
  /// stream/future ops: the element type declared at the built-in, and the
  /// instance its type indices belong to when the type was aliased.
  std::optional<ComponentValType> Elem;
  const Runtime::Instance::ComponentInstance *ElemInst = nullptr;
  bool IsStream = true;
  /// context.get/set slot index and slot type.
  uint32_t ContextIdx = 0;
  ValType ContextType = TypeCode::I32;
  /// task.return: declared result types.
  std::vector<ComponentValType> RetTypes;
  /// thread.new-indirect: the core table holding start functions.
  Runtime::Instance::TableInstance *Table = nullptr;

  bool isAsync() const noexcept { return Flag; }
  bool isCancellable() const noexcept { return Flag; }
  /// The instance the element type indices belong to.
  const Runtime::Instance::ComponentInstance *getElemInstance() const noexcept {
    return ElemInst != nullptr ? ElemInst : Opts.Inst;
  }
};

class CanonAsyncBuiltinHostFunc : public Runtime::HostFunctionBase {
public:
  CanonAsyncBuiltinHostFunc(ComponentExecutor *CompExec,
                            AsyncBuiltinInfo BuiltinInfo) noexcept;

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
