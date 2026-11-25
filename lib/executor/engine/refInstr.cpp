// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/executor.h"

#include <cstring>
#include <vector>

namespace WasmEdge {
namespace Executor {

namespace {

template <typename... T>
ErrCode logError(const ErrCode &Code, const AST::Instruction &Instr,
                 T &&...F) noexcept {
  spdlog::error(Code);
  (F(), ...);
  spdlog::error(ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
  return Code;
}

ErrCode logError(const ErrCode &Code, const AST::Instruction &Instr) noexcept {
  spdlog::error(Code);
  spdlog::error(ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
  return Code;
}

constexpr uint32_t kFieldBase = sizeof(Runtime::HeapHeader);

/// Checked address of `Bytes` bytes at the field position `Pos` of `View`.
Expect<uint8_t *> getFieldSpan(const Runtime::RefHeap::DataView &View,
                               uint64_t Pos, uint64_t Bytes) noexcept {
  if (auto *P = View.getSpan(kFieldBase + Pos, Bytes)) {
    return P;
  }
  return Unexpect(ErrCode::Value::MemoryOutOfBounds);
}

uint32_t arrayLength(const RefVariant &Ref) noexcept {
  auto *Heap = Runtime::RefHeap::getGlobal();
  const uint32_t Off = Heap != nullptr ? Ref.getOffset() : 0;
  const auto *Header = Off != 0 ? Heap->getHeader(Off) : nullptr;
  return Header != nullptr ? Header->Length : 0;
}

void writeField(uint8_t *Dst, const ValType &Type,
                const ValVariant &Val) noexcept {
  auto Write = [Dst](auto V) noexcept { std::memcpy(Dst, &V, sizeof(V)); };
  switch (Type.getCode()) {
  case TypeCode::I8:
    return Write(static_cast<uint8_t>(Val.get<uint32_t>()));
  case TypeCode::I16:
    return Write(static_cast<uint16_t>(Val.get<uint32_t>()));
  case TypeCode::I32:
    return Write(Val.get<uint32_t>());
  case TypeCode::F32:
    return Write(Val.get<float>());
  case TypeCode::I64:
    return Write(Val.get<uint64_t>());
  case TypeCode::F64:
    return Write(Val.get<double>());
  case TypeCode::V128:
    return Write(Val.get<uint128_t>());
  default:
    return Write(Val.get<RefVariant>());
  }
}

ValVariant readField(const uint8_t *Src, const ValType &Type,
                     const bool IsSigned) noexcept {
  auto Read = [Src](auto V) noexcept {
    std::memcpy(&V, Src, sizeof(V));
    return V;
  };
  ValVariant Val(static_cast<uint128_t>(0U));
  switch (Type.getCode()) {
  case TypeCode::I8: {
    const auto V = Read(uint8_t(0));
    Val.emplace<uint32_t>(IsSigned ? (static_cast<uint32_t>(V) ^ 0x80U) - 0x80U
                                   : static_cast<uint32_t>(V));
    break;
  }
  case TypeCode::I16: {
    const auto V = Read(uint16_t(0));
    Val.emplace<uint32_t>(IsSigned
                              ? (static_cast<uint32_t>(V) ^ 0x8000U) - 0x8000U
                              : static_cast<uint32_t>(V));
    break;
  }
  case TypeCode::I32:
    Val.emplace<uint32_t>(Read(uint32_t(0)));
    break;
  case TypeCode::F32:
    Val.emplace<float>(Read(0.0f));
    break;
  case TypeCode::I64:
    Val.emplace<uint64_t>(Read(uint64_t(0)));
    break;
  case TypeCode::F64:
    Val.emplace<double>(Read(0.0));
    break;
  case TypeCode::V128:
    Val.emplace<uint128_t>(Read(static_cast<uint128_t>(0U)));
    break;
  default:
    Val.emplace<RefVariant>(Read(RefVariant()));
    break;
  }
  return Val;
}

/// Bytes of `Cnt` elements of `Size` bytes from `Start` in the data segment,
/// or MemoryOutOfBounds if the segment is too short.
Expect<const uint8_t *>
getSegmentBytes(const Runtime::Instance::DataInstance &Data,
                const uint32_t Start, const uint32_t Cnt,
                const uint32_t Size) noexcept {
  if (static_cast<uint64_t>(Start) + static_cast<uint64_t>(Cnt) * Size >
      Data.getData().size()) {
    return Unexpect(ErrCode::Value::MemoryOutOfBounds);
  }
  return Data.getData().data() + Start;
}

/// Copy `Cnt` little-endian elements of `Size` bytes from a data segment.
void copySegment(uint8_t *Dst, const uint8_t *Src, const uint32_t Cnt,
                 const uint32_t Size) noexcept {
  if constexpr (Endian::native == Endian::little) {
    if (Cnt != 0) {
      std::memcpy(Dst, Src, static_cast<size_t>(Cnt) * Size);
    }
  } else {
    auto Copy = [&](auto X) noexcept {
      for (uint32_t I = 0; I < Cnt; I++) {
        EndianValue<decltype(X)> V;
        std::memcpy(&V.raw(), Src + static_cast<size_t>(I) * Size, sizeof(X));
        X = V.le();
        std::memcpy(Dst + static_cast<size_t>(I) * Size, &X, sizeof(X));
      }
    };
    switch (Size) {
    case 1:
      return Copy(uint8_t(0));
    case 2:
      return Copy(uint16_t(0));
    case 4:
      return Copy(uint32_t(0));
    case 8:
      return Copy(uint64_t(0));
    default:
      return Copy(static_cast<uint128_t>(0U));
    }
  }
}

void logArrayOOB(const ErrCode &Code, const uint32_t Idx, const uint32_t Cnt,
                 const RefVariant &Ref) noexcept {
  if (Code == ErrCode::Value::ArrayOutOfBounds) {
    spdlog::error(ErrInfo::InfoBoundary(static_cast<uint64_t>(Idx), Cnt,
                                        arrayLength(Ref)));
  }
}

void logDoubleArrayOOB(const ErrCode &Code, const uint32_t Idx1,
                       const uint32_t Cnt1, const RefVariant &Ref1,
                       const uint32_t Idx2, const uint32_t Cnt2,
                       const RefVariant &Ref2) noexcept {
  if (Code == ErrCode::Value::ArrayOutOfBounds) {
    const uint32_t Len1 = arrayLength(Ref1);
    const uint32_t Len2 = arrayLength(Ref2);
    if (static_cast<uint64_t>(Idx1) + static_cast<uint64_t>(Cnt1) > Len1) {
      spdlog::error(
          ErrInfo::InfoBoundary(static_cast<uint64_t>(Idx1), Cnt1, Len1));
    } else if (static_cast<uint64_t>(Idx2) + static_cast<uint64_t>(Cnt2) >
               Len2) {
      spdlog::error(
          ErrInfo::InfoBoundary(static_cast<uint64_t>(Idx2), Cnt2, Len2));
    }
  }
}

void logMemoryOOB(const ErrCode &Code,
                  const Runtime::Instance::DataInstance &DataInst,
                  const uint32_t Idx, const uint32_t Length) noexcept {
  if (Code == ErrCode::Value::MemoryOutOfBounds) {
    spdlog::error(ErrInfo::InfoBoundary(
        static_cast<uint64_t>(Idx), Length,
        static_cast<uint32_t>(DataInst.getData().size())));
  }
}

void logTableOOB(const ErrCode &Code,
                 const Runtime::Instance::ElementInstance &ElemInst,
                 const uint32_t Idx, const uint32_t Length) noexcept {
  if (Code == ErrCode::Value::TableOutOfBounds) {
    auto ElemSrc = ElemInst.getRefs();
    spdlog::error(ErrInfo::InfoBoundary(static_cast<uint64_t>(Idx), Length,
                                        static_cast<uint32_t>(ElemSrc.size())));
  }
}

} // namespace

Expect<void> Executor::runRefNullOp(Runtime::StackManager &StackMgr,
                                    const ValType &Type) const noexcept {
  // A null reference is typed with the least type in its respective hierarchy.
  StackMgr.push(RefVariant(toBottomType(StackMgr.getModule(), Type)));
  return {};
}

Expect<void> Executor::runRefIsNullOp(ValVariant &Val) const noexcept {
  Val.emplace<uint32_t>(Val.get<RefVariant>().isNull() ? 1U : 0U);
  return {};
}

Expect<void> Executor::runRefFuncOp(Runtime::StackManager &StackMgr,
                                    uint32_t Idx) const noexcept {
  const auto *FuncInst = getFuncInstByIdx(StackMgr.getModule(), Idx);
  StackMgr.push(RefVariant::makeFunc(ValType(TypeCode::Ref, TypeCode::FuncRef),
                                     FuncInst));
  return {};
}

Expect<void> Executor::runRefEqOp(ValVariant &Val1,
                                  const ValVariant &Val2) const noexcept {
  Val1.emplace<uint32_t>(
      Val1.get<RefVariant>().isSameRef(Val2.get<RefVariant>()) ? 1U : 0U);
  return {};
}

Expect<void>
Executor::runRefAsNonNullOp(RefVariant &Ref,
                            const AST::Instruction &Instr) const noexcept {
  if (Ref.isNull()) {
    return Unexpect(logError(ErrCode::Value::CastNullToNonNull, Instr));
  }
  return {};
}

Expect<void> Executor::runStructNewOp(Runtime::StackManager &StackMgr,
                                      const uint32_t TypeIdx,
                                      const AST::Instruction &Instr,
                                      const bool IsDefault) const noexcept {
  uint32_t N = 0;
  if (!IsDefault) {
    const auto &CompType = getCompositeTypeByIdx(StackMgr.getModule(), TypeIdx);
    N = static_cast<uint32_t>(CompType.getFieldTypes().size());
  }
  EXPECTED_TRY(auto Ref,
               structNew(StackMgr.getModule(), TypeIdx, StackMgr.getTopSpan(N))
                   .map_error([&](auto E) { return logError(E, Instr); }));
  StackMgr.eraseValueStack(N, 0);
  StackMgr.push(Ref);
  return {};
}

Expect<void> Executor::runStructGetOp(Runtime::StackManager &StackMgr,
                                      const uint32_t TypeIdx,
                                      const uint32_t Off,
                                      const AST::Instruction &Instr,
                                      const bool IsSigned) const noexcept {
  const RefVariant Ref = StackMgr.getTop().get<RefVariant>();
  EXPECTED_TRY(auto Val,
               structGet(StackMgr.getModule(), Ref, TypeIdx, Off, IsSigned)
                   .map_error([&](auto E) { return logError(E, Instr); }));
  StackMgr.getTop() = Val;
  return {};
}

Expect<void>
Executor::runStructSetOp(Runtime::StackManager &StackMgr, const ValVariant &Val,
                         const uint32_t TypeIdx, const uint32_t Off,
                         const AST::Instruction &Instr) const noexcept {
  const RefVariant Ref = StackMgr.pop().get<RefVariant>();
  EXPECTED_TRY(structSet(StackMgr.getModule(), Ref, Val, TypeIdx, Off)
                   .map_error([&](auto E) { return logError(E, Instr); }));
  return {};
}

Expect<void>
Executor::runArrayNewOp(Runtime::StackManager &StackMgr, const uint32_t TypeIdx,
                        const uint32_t InitCnt, uint32_t Length,
                        const AST::Instruction &Instr) const noexcept {
  assuming(InitCnt == 0 || InitCnt == 1 || InitCnt == Length);
  auto Report = [&](auto E) { return logError(E, Instr); };
  if (InitCnt == 1) {
    EXPECTED_TRY(auto Ref, arrayNew(StackMgr.getModule(), TypeIdx, Length,
                                    {StackMgr.getTop()})
                               .map_error(Report));
    StackMgr.getTop().emplace<RefVariant>(Ref);
    return {};
  }
  const uint32_t N = InitCnt > 1 ? Length : 0;
  EXPECTED_TRY(auto Ref, arrayNew(StackMgr.getModule(), TypeIdx, Length,
                                  StackMgr.getTopSpan(N))
                             .map_error(Report));
  StackMgr.eraseValueStack(N, 0);
  StackMgr.push(Ref);
  return {};
}

Expect<void>
Executor::runArrayNewDataOp(Runtime::StackManager &StackMgr,
                            const uint32_t TypeIdx, const uint32_t DataIdx,
                            const AST::Instruction &Instr) const noexcept {
  const uint32_t Length = StackMgr.pop().get<uint32_t>();
  const uint32_t Start = StackMgr.getTop().get<uint32_t>();
  EXPECTED_TRY(
      auto InstRef,
      arrayNewData(StackMgr.getModule(), TypeIdx, DataIdx, Start, Length)
          .map_error([&](auto E) {
            auto *DataInst = getDataInstByIdx(StackMgr.getModule(), DataIdx);
            const uint32_t BSize =
                getArrayStorageTypeByIdx(StackMgr.getModule(), TypeIdx)
                    .getBitWidth() /
                8;
            return logError(E, Instr, [&]() {
              return logMemoryOOB(E, *DataInst, Start, BSize * Length);
            });
          }));
  StackMgr.getTop().emplace<RefVariant>(InstRef);
  return {};
}

Expect<void>
Executor::runArrayNewElemOp(Runtime::StackManager &StackMgr,
                            const uint32_t TypeIdx, const uint32_t ElemIdx,
                            const AST::Instruction &Instr) const noexcept {
  const uint32_t Length = StackMgr.pop().get<uint32_t>();
  const uint32_t Start = StackMgr.getTop().get<uint32_t>();
  EXPECTED_TRY(
      auto InstRef,
      arrayNewElem(StackMgr.getModule(), TypeIdx, ElemIdx, Start, Length)
          .map_error([&](auto E) {
            auto *ElemInst = getElemInstByIdx(StackMgr.getModule(), ElemIdx);
            return logError(E, Instr, [&]() {
              return logTableOOB(E, *ElemInst, Start, Length);
            });
          }));
  StackMgr.getTop().emplace<RefVariant>(InstRef);
  return {};
}

Expect<void> Executor::runArrayGetOp(Runtime::StackManager &StackMgr,
                                     const uint32_t TypeIdx,
                                     const AST::Instruction &Instr,
                                     const bool IsSigned) const noexcept {
  const uint32_t Idx = StackMgr.pop().get<uint32_t>();
  const RefVariant Ref = StackMgr.getTop().get<RefVariant>();
  EXPECTED_TRY(auto Val,
               arrayGet(StackMgr.getModule(), Ref, TypeIdx, Idx, IsSigned)
                   .map_error([&](auto E) {
                     return logError(E, Instr, [&]() {
                       return logArrayOOB(E, Idx, 1, Ref);
                     });
                   }));
  StackMgr.getTop() = Val;
  return {};
}

Expect<void>
Executor::runArraySetOp(Runtime::StackManager &StackMgr, const ValVariant &Val,
                        const uint32_t TypeIdx,
                        const AST::Instruction &Instr) const noexcept {
  const uint32_t Idx = StackMgr.pop().get<uint32_t>();
  const RefVariant Ref = StackMgr.pop().get<RefVariant>();
  EXPECTED_TRY(arraySet(StackMgr.getModule(), Ref, Val, TypeIdx, Idx)
                   .map_error([&](auto E) {
                     return logError(E, Instr, [&]() {
                       return logArrayOOB(E, Idx, 1, Ref);
                     });
                   }));
  return {};
}

Expect<void>
Executor::runArrayLenOp(ValVariant &Val,
                        const AST::Instruction &Instr) const noexcept {
  EXPECTED_TRY(auto Len, arrayLen(Val.get<RefVariant>()).map_error([&](auto E) {
    return logError(E, Instr);
  }));
  Val.emplace<uint32_t>(Len);
  return {};
}

Expect<void>
Executor::runArrayFillOp(Runtime::StackManager &StackMgr, const uint32_t Cnt,
                         const ValVariant &Val, const uint32_t TypeIdx,
                         const AST::Instruction &Instr) const noexcept {
  const uint32_t Idx = StackMgr.pop().get<uint32_t>();
  const RefVariant Ref = StackMgr.pop().get<RefVariant>();
  EXPECTED_TRY(arrayFill(StackMgr.getModule(), Ref, Val, TypeIdx, Idx, Cnt)
                   .map_error([&](auto E) {
                     return logError(E, Instr, [&]() {
                       return logArrayOOB(E, Idx, Cnt, Ref);
                     });
                   }));
  return {};
}

Expect<void>
Executor::runArrayCopyOp(Runtime::StackManager &StackMgr, const uint32_t Cnt,
                         const uint32_t DstTypeIdx, const uint32_t SrcTypeIdx,
                         const AST::Instruction &Instr) const noexcept {
  const uint32_t SrcIdx = StackMgr.pop().get<uint32_t>();
  const RefVariant SrcRef = StackMgr.pop().get<RefVariant>();
  const uint32_t DstIdx = StackMgr.pop().get<uint32_t>();
  const RefVariant DstRef = StackMgr.pop().get<RefVariant>();
  EXPECTED_TRY(arrayCopy(StackMgr.getModule(), DstRef, DstTypeIdx, DstIdx,
                         SrcRef, SrcTypeIdx, SrcIdx, Cnt)
                   .map_error([&](auto E) {
                     return logError(E, Instr, [&]() {
                       return logDoubleArrayOOB(E, SrcIdx, Cnt, SrcRef, DstIdx,
                                                Cnt, DstRef);
                     });
                   }));
  return {};
}

Expect<void> Executor::runArrayInitDataOp(
    Runtime::StackManager &StackMgr, const uint32_t Cnt, const uint32_t TypeIdx,
    const uint32_t DataIdx, const AST::Instruction &Instr) const noexcept {
  const uint32_t SrcIdx = StackMgr.pop().get<uint32_t>();
  const uint32_t DstIdx = StackMgr.pop().get<uint32_t>();
  const RefVariant Ref = StackMgr.pop().get<RefVariant>();
  EXPECTED_TRY(
      arrayInitData(StackMgr.getModule(), Ref, TypeIdx, DataIdx, DstIdx, SrcIdx,
                    Cnt)
          .map_error([&](auto E) {
            auto *DataInst = getDataInstByIdx(StackMgr.getModule(), DataIdx);
            const uint32_t BSize =
                getArrayStorageTypeByIdx(StackMgr.getModule(), TypeIdx)
                    .getBitWidth() /
                8;
            return logError(
                E, Instr, [&]() { return logArrayOOB(E, DstIdx, Cnt, Ref); },
                [&]() {
                  return logMemoryOOB(E, *DataInst, SrcIdx, Cnt * BSize);
                });
          }));
  return {};
}

Expect<void> Executor::runArrayInitElemOp(
    Runtime::StackManager &StackMgr, const uint32_t Cnt, const uint32_t TypeIdx,
    const uint32_t ElemIdx, const AST::Instruction &Instr) const noexcept {
  const uint32_t SrcIdx = StackMgr.pop().get<uint32_t>();
  const uint32_t DstIdx = StackMgr.pop().get<uint32_t>();
  const RefVariant Ref = StackMgr.pop().get<RefVariant>();
  EXPECTED_TRY(
      arrayInitElem(StackMgr.getModule(), Ref, TypeIdx, ElemIdx, DstIdx, SrcIdx,
                    Cnt)
          .map_error([&](auto E) {
            auto *ElemInst = getElemInstByIdx(StackMgr.getModule(), ElemIdx);
            return logError(
                E, Instr, [&]() { return logArrayOOB(E, DstIdx, Cnt, Ref); },
                [&]() { return logTableOOB(E, *ElemInst, SrcIdx, Cnt); });
          }));
  return {};
}

Expect<void>
Executor::runRefTestOp(const Runtime::Instance::ModuleInstance *ModInst,
                       ValVariant &Val, const AST::Instruction &Instr,
                       const bool IsCast) const noexcept {
  if (Runtime::Matcher::matchRef(ModInst, Instr.getValType(),
                                 Val.get<RefVariant>())) {
    if (!IsCast) {
      Val.emplace<uint32_t>(1U);
    }
  } else {
    if (IsCast) {
      spdlog::error(ErrCode::Value::CastFailed);
      spdlog::error(ErrInfo::InfoMismatch(Instr.getValType(),
                                          Val.get<RefVariant>().getType()));
      spdlog::error(
          ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
      return Unexpect(ErrCode::Value::CastFailed);
    } else {
      Val.emplace<uint32_t>(0U);
    }
  }
  return {};
}

Expect<void> Executor::runRefConvOp(RefVariant &Ref,
                                    TypeCode TCode) const noexcept {
  // A non-null reference is the same value seen from inside and from outside.
  if (Ref.isNull()) {
    Ref = RefVariant(ValType(TypeCode::RefNull, TCode == TypeCode::AnyRef
                                                    ? TypeCode::NullRef
                                                    : TypeCode::NullExternRef));
  }
  return {};
}

Expect<void> Executor::runRefI31Op(ValVariant &Val) const noexcept {
  Val = RefVariant::makeI31(Val.get<uint32_t>());
  return {};
}

Expect<void> Executor::runI31GetOp(ValVariant &Val,
                                   const AST::Instruction &Instr,
                                   const bool IsSigned) const noexcept {
  const auto Payload = Val.get<RefVariant>().getI31();
  if (!Payload) {
    return Unexpect(logError(ErrCode::Value::AccessNullI31, Instr));
  }
  uint32_t RefNum = *Payload;
  if (IsSigned) {
    RefNum = (RefNum ^ 0x40000000U) - 0x40000000U;
  }
  Val.emplace<uint32_t>(RefNum);
  return {};
}

Expect<RefVariant>
Executor::structNew(const Runtime::Instance::ModuleInstance *ModInstArg,
                    const uint32_t TypeIdx,
                    Span<const ValVariant> Args) const noexcept {
  /// TODO: The array and struct instances are currently owned by the module
  /// instance because they refer to the defined types of the module instances.
  /// This may be changed after applying the garbage collection mechanism.
  const auto &CompType = getCompositeTypeByIdx(ModInstArg, TypeIdx);
  const auto &FieldTypes = CompType.getFieldTypes();
  const auto N = static_cast<uint32_t>(FieldTypes.size());
  const auto Positions =
      Runtime::CanonicalTypeTable::getGlobal().getFieldPositions(
          ModInstArg->getCanonicalTypeId(TypeIdx));
  assuming(Positions.size() == N + 1U);
  auto *ModInst = const_cast<Runtime::Instance::ModuleInstance *>(ModInstArg);
  EXPECTED_TRY(const auto View, ModInst->newStruct(TypeIdx, Positions[N]));
  EXPECTED_TRY(uint8_t *Fields, getFieldSpan(View, 0, Positions[N]));
  for (uint32_t I = 0; I < N; I++) {
    const auto &VType = FieldTypes[I].getStorageType();
    if (!Args.empty()) {
      writeField(Fields + Positions[I], VType, Args[I]);
    } else if (VType.isRefType()) {
      writeField(Fields + Positions[I], VType,
                 RefVariant(toBottomType(ModInstArg, VType)));
    }
  }
  return RefVariant::makeStruct(ValType(TypeCode::Ref, TypeIdx),
                                View.getOffset());
}

Expect<ValVariant>
Executor::structGet(const Runtime::Instance::ModuleInstance *ModInst,
                    const RefVariant Ref, const uint32_t TypeIdx,
                    const uint32_t Off, const bool IsSigned) const noexcept {
  EXPECTED_TRY(const auto View, getDataView(Ref, Runtime::HeapKind::Struct));
  const auto &VType = getStructStorageTypeByIdx(ModInst, TypeIdx, Off);
  const auto Positions =
      Runtime::CanonicalTypeTable::getGlobal().getFieldPositions(
          ModInst->getCanonicalTypeId(TypeIdx));
  assuming(Off < Positions.size());
  EXPECTED_TRY(const uint8_t *Field,
               getFieldSpan(View, Positions[Off],
                            Runtime::CanonicalTypeTable::getFieldSize(VType)));
  return readField(Field, VType, IsSigned);
}

Expect<void>
Executor::structSet(const Runtime::Instance::ModuleInstance *ModInst,
                    const RefVariant Ref, const ValVariant Val,
                    const uint32_t TypeIdx, const uint32_t Off) const noexcept {
  EXPECTED_TRY(const auto View, getDataView(Ref, Runtime::HeapKind::Struct));
  const auto &VType = getStructStorageTypeByIdx(ModInst, TypeIdx, Off);
  const auto Positions =
      Runtime::CanonicalTypeTable::getGlobal().getFieldPositions(
          ModInst->getCanonicalTypeId(TypeIdx));
  assuming(Off < Positions.size());
  EXPECTED_TRY(uint8_t *Field,
               getFieldSpan(View, Positions[Off],
                            Runtime::CanonicalTypeTable::getFieldSize(VType)));
  writeField(Field, VType, Val);
  return {};
}

namespace {

/// Checked span of all elements of the new array that `View` shows.
Expect<uint8_t *> getNewElems(const Runtime::RefHeap::DataView &View,
                              const uint32_t Length,
                              const uint32_t ElemSize) noexcept {
  return getFieldSpan(View, 0, static_cast<uint64_t>(Length) * ElemSize);
}

} // namespace

Expect<RefVariant>
Executor::arrayNew(const Runtime::Instance::ModuleInstance *ModInst,
                   const uint32_t TypeIdx, const uint32_t Length,
                   Span<const ValVariant> Args) const noexcept {
  /// TODO: The array and struct instances are currently owned by the module
  /// instance because they refer to the defined types of the module instances.
  /// This may be changed after applying the garbage collection mechanism.
  const auto &VType = getArrayStorageTypeByIdx(ModInst, TypeIdx);
  const uint32_t ElemSize = Runtime::CanonicalTypeTable::getFieldSize(VType);
  auto *Owner = const_cast<Runtime::Instance::ModuleInstance *>(ModInst);
  EXPECTED_TRY(const auto View, Owner->newArray(TypeIdx, Length, ElemSize));
  EXPECTED_TRY(uint8_t *Elems, getNewElems(View, Length, ElemSize));
  if (Args.size() > 1) {
    for (uint32_t I = 0; I < Length; I++) {
      writeField(Elems + I * ElemSize, VType, Args[I]);
    }
  } else if (Args.size() == 1 || VType.isRefType()) {
    const ValVariant Init =
        Args.size() == 1 ? Args[0]
                         : ValVariant(RefVariant(toBottomType(ModInst, VType)));
    for (uint32_t I = 0; I < Length; I++) {
      writeField(Elems + I * ElemSize, VType, Init);
    }
  }
  return RefVariant::makeArray(ValType(TypeCode::Ref, TypeIdx),
                               View.getOffset());
}

Expect<RefVariant>
Executor::arrayNewData(const Runtime::Instance::ModuleInstance *ModInst,
                       const uint32_t TypeIdx, const uint32_t DataIdx,
                       const uint32_t Start,
                       const uint32_t Length) const noexcept {
  const auto &VType = getArrayStorageTypeByIdx(ModInst, TypeIdx);
  const uint32_t BSize = VType.getBitWidth() / 8;
  auto *DataInst = getDataInstByIdx(ModInst, DataIdx);
  assuming(DataInst);
  EXPECTED_TRY(const uint8_t *Src,
               getSegmentBytes(*DataInst, Start, Length, BSize));
  auto *Owner = const_cast<Runtime::Instance::ModuleInstance *>(ModInst);
  EXPECTED_TRY(const auto View, Owner->newArray(TypeIdx, Length, BSize));
  EXPECTED_TRY(uint8_t *Elems, getNewElems(View, Length, BSize));
  copySegment(Elems, Src, Length, BSize);
  return RefVariant::makeArray(ValType(TypeCode::Ref, TypeIdx),
                               View.getOffset());
}

Expect<RefVariant>
Executor::arrayNewElem(const Runtime::Instance::ModuleInstance *ModInst,
                       const uint32_t TypeIdx, const uint32_t ElemIdx,
                       const uint32_t Start,
                       const uint32_t Length) const noexcept {
  const auto &VType = getArrayStorageTypeByIdx(ModInst, TypeIdx);
  auto *ElemInst = getElemInstByIdx(ModInst, ElemIdx);
  assuming(ElemInst);
  auto ElemSrc = ElemInst->getRefs();
  if (static_cast<uint64_t>(Start) + static_cast<uint64_t>(Length) >
      ElemSrc.size()) {
    return Unexpect(ErrCode::Value::TableOutOfBounds);
  }
  const uint32_t ElemSize = Runtime::CanonicalTypeTable::getFieldSize(VType);
  auto *Owner = const_cast<Runtime::Instance::ModuleInstance *>(ModInst);
  EXPECTED_TRY(const auto View, Owner->newArray(TypeIdx, Length, ElemSize));
  EXPECTED_TRY(uint8_t *Elems, getNewElems(View, Length, ElemSize));
  for (uint32_t I = 0; I < Length; I++) {
    writeField(Elems + I * ElemSize, VType, ElemSrc[Start + I]);
  }
  return RefVariant::makeArray(ValType(TypeCode::Ref, TypeIdx),
                               View.getOffset());
}

Expect<uint32_t> Executor::arrayLen(const RefVariant &Ref) const noexcept {
  EXPECTED_TRY(const auto View, getDataView(Ref, Runtime::HeapKind::Array));
  return View.Header->Length;
}

namespace {

/// Checked span of `Cnt` elements of `ElemSize` bytes from `Idx` in the array
/// of `View`.
Expect<uint8_t *> getElemSpan(const Runtime::RefHeap::DataView &View,
                              const uint32_t Idx, const uint32_t Cnt,
                              const uint32_t ElemSize) noexcept {
  if (static_cast<uint64_t>(Idx) + static_cast<uint64_t>(Cnt) >
      View.Header->Length) {
    return Unexpect(ErrCode::Value::ArrayOutOfBounds);
  }
  return getFieldSpan(View, static_cast<uint64_t>(Idx) * ElemSize,
                      static_cast<uint64_t>(Cnt) * ElemSize);
}

} // namespace

Expect<ValVariant>
Executor::arrayGet(const Runtime::Instance::ModuleInstance *ModInst,
                   const RefVariant &Ref, const uint32_t TypeIdx,
                   const uint32_t Idx, const bool IsSigned) const noexcept {
  EXPECTED_TRY(const auto View, getDataView(Ref, Runtime::HeapKind::Array));
  const auto &VType = getArrayStorageTypeByIdx(ModInst, TypeIdx);
  EXPECTED_TRY(const uint8_t *Elem,
               getElemSpan(View, Idx, 1,
                           Runtime::CanonicalTypeTable::getFieldSize(VType)));
  return readField(Elem, VType, IsSigned);
}

Expect<void>
Executor::arraySet(const Runtime::Instance::ModuleInstance *ModInst,
                   const RefVariant &Ref, const ValVariant &Val,
                   const uint32_t TypeIdx, const uint32_t Idx) const noexcept {
  EXPECTED_TRY(const auto View, getDataView(Ref, Runtime::HeapKind::Array));
  const auto &VType = getArrayStorageTypeByIdx(ModInst, TypeIdx);
  EXPECTED_TRY(uint8_t *Elem,
               getElemSpan(View, Idx, 1,
                           Runtime::CanonicalTypeTable::getFieldSize(VType)));
  writeField(Elem, VType, Val);
  return {};
}

Expect<void>
Executor::arrayFill(const Runtime::Instance::ModuleInstance *ModInst,
                    const RefVariant &Ref, const ValVariant &Val,
                    const uint32_t TypeIdx, const uint32_t Idx,
                    const uint32_t Cnt) const noexcept {
  EXPECTED_TRY(const auto View, getDataView(Ref, Runtime::HeapKind::Array));
  const auto &VType = getArrayStorageTypeByIdx(ModInst, TypeIdx);
  const uint32_t ElemSize = Runtime::CanonicalTypeTable::getFieldSize(VType);
  EXPECTED_TRY(uint8_t *Elems, getElemSpan(View, Idx, Cnt, ElemSize));
  for (uint32_t I = 0; I < Cnt; I++) {
    writeField(Elems + static_cast<size_t>(I) * ElemSize, VType, Val);
  }
  return {};
}

Expect<void> Executor::arrayInitData(
    const Runtime::Instance::ModuleInstance *ModInst, const RefVariant &Ref,
    const uint32_t TypeIdx, const uint32_t DataIdx, const uint32_t DstIdx,
    const uint32_t SrcIdx, const uint32_t Cnt) const noexcept {
  EXPECTED_TRY(const auto View, getDataView(Ref, Runtime::HeapKind::Array));
  const auto &VType = getArrayStorageTypeByIdx(ModInst, TypeIdx);
  const uint32_t BSize = VType.getBitWidth() / 8;
  EXPECTED_TRY(uint8_t *Elems, getElemSpan(View, DstIdx, Cnt, BSize));
  auto *DataInst = getDataInstByIdx(ModInst, DataIdx);
  assuming(DataInst);
  EXPECTED_TRY(const uint8_t *Src,
               getSegmentBytes(*DataInst, SrcIdx, Cnt, BSize));
  copySegment(Elems, Src, Cnt, BSize);
  return {};
}

Expect<void> Executor::arrayInitElem(
    const Runtime::Instance::ModuleInstance *ModInst, const RefVariant &Ref,
    const uint32_t TypeIdx, const uint32_t ElemIdx, const uint32_t DstIdx,
    const uint32_t SrcIdx, const uint32_t Cnt) const noexcept {
  EXPECTED_TRY(const auto View, getDataView(Ref, Runtime::HeapKind::Array));
  const auto &VType = getArrayStorageTypeByIdx(ModInst, TypeIdx);
  const uint32_t ElemSize = Runtime::CanonicalTypeTable::getFieldSize(VType);
  EXPECTED_TRY(uint8_t *Elems, getElemSpan(View, DstIdx, Cnt, ElemSize));
  auto *ElemInst = getElemInstByIdx(ModInst, ElemIdx);
  assuming(ElemInst);
  auto ElemSrc = ElemInst->getRefs();
  if (static_cast<uint64_t>(SrcIdx) + static_cast<uint64_t>(Cnt) >
      ElemSrc.size()) {
    return Unexpect(ErrCode::Value::TableOutOfBounds);
  }
  for (uint32_t I = 0; I < Cnt; I++) {
    writeField(Elems + static_cast<size_t>(I) * ElemSize, VType,
               ElemSrc[SrcIdx + I]);
  }
  return {};
}

Expect<void>
Executor::arrayCopy(const Runtime::Instance::ModuleInstance *ModInst,
                    const RefVariant &DstRef, const uint32_t DstTypeIdx,
                    const uint32_t DstIdx, const RefVariant &SrcRef,
                    const uint32_t SrcTypeIdx, const uint32_t SrcIdx,
                    const uint32_t Cnt) const noexcept {
  EXPECTED_TRY(const auto SrcView,
               getDataView(SrcRef, Runtime::HeapKind::Array));
  EXPECTED_TRY(const auto DstView,
               getDataView(DstRef, Runtime::HeapKind::Array));
  const uint32_t ElemSize = Runtime::CanonicalTypeTable::getFieldSize(
      getArrayStorageTypeByIdx(ModInst, DstTypeIdx));
  assuming(ElemSize == Runtime::CanonicalTypeTable::getFieldSize(
                           getArrayStorageTypeByIdx(ModInst, SrcTypeIdx)));
  EXPECTED_TRY(uint8_t *Src, getElemSpan(SrcView, SrcIdx, Cnt, ElemSize));
  EXPECTED_TRY(uint8_t *Dst, getElemSpan(DstView, DstIdx, Cnt, ElemSize));
  std::memmove(Dst, Src, static_cast<size_t>(Cnt) * ElemSize);
  return {};
}

} // namespace Executor
} // namespace WasmEdge
