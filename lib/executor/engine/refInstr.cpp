// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

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

void logArrayOOB(const ErrCode &Code, const uint32_t Idx, const uint32_t Cnt,
                 const RefVariant &Ref) noexcept {
  if (Code == ErrCode::Value::ArrayOutOfBounds) {
    const auto *Inst = Ref.getPtr<Runtime::Instance::ArrayInstance>();
    spdlog::error(ErrInfo::InfoBoundary(static_cast<uint64_t>(Idx), Cnt,
                                        Inst->getBoundIdx()));
  }
}

void logDoubleArrayOOB(const ErrCode &Code, const uint32_t Idx1,
                       const uint32_t Cnt1, const RefVariant &Ref1,
                       const uint32_t Idx2, const uint32_t Cnt2,
                       const RefVariant &Ref2) noexcept {
  if (Code == ErrCode::Value::ArrayOutOfBounds) {
    const auto *Inst1 = Ref1.getPtr<Runtime::Instance::ArrayInstance>();
    const auto *Inst2 = Ref2.getPtr<Runtime::Instance::ArrayInstance>();
    if (static_cast<uint64_t>(Idx1) + static_cast<uint64_t>(Cnt1) >
        Inst1->getLength()) {
      spdlog::error(ErrInfo::InfoBoundary(static_cast<uint64_t>(Idx1), Cnt1,
                                          Inst1->getBoundIdx()));
    } else if (static_cast<uint64_t>(Idx2) + static_cast<uint64_t>(Cnt2) >
               Inst2->getLength()) {
      spdlog::error(ErrInfo::InfoBoundary(static_cast<uint64_t>(Idx2), Cnt2,
                                          Inst2->getBoundIdx()));
    }
  }
}

void logMemoryOOB(const ErrCode &Code,
                  const Runtime::Instance::DataInstance &DataInst,
                  const uint32_t Idx, const uint32_t Length) noexcept {
  if (Code == ErrCode::Value::MemoryOutOfBounds) {
    spdlog::error(ErrInfo::InfoBoundary(
        static_cast<uint64_t>(Idx), Length,
        DataInst.getData().size() > 0
            ? static_cast<uint32_t>(DataInst.getData().size() - 1)
            : 0U));
  }
}

void logTableOOB(const ErrCode &Code,
                 const Runtime::Instance::ElementInstance &ElemInst,
                 const uint32_t Idx, const uint32_t Length) noexcept {
  if (Code == ErrCode::Value::TableOutOfBounds) {
    auto ElemSrc = ElemInst.getRefs();
    spdlog::error(ErrInfo::InfoBoundary(
        static_cast<uint64_t>(Idx), Length,
        ElemSrc.size() > 0 ? static_cast<uint32_t>(ElemSrc.size() - 1) : 0U));
  }
}

} // namespace

Expect<void> Executor::runRefNullOp(Runtime::StackManager &StackMgr,
                                    const ValType &Type) const noexcept {
  // A null reference is typed with the least type in its respective hierarchy.
  StackMgr.push(RefVariant(toBottomType(StackMgr, Type)));
  return {};
}

Expect<void> Executor::runRefIsNullOp(ValVariant &Val) const noexcept {
  Val.emplace<uint32_t>(Val.get<RefVariant>().isNull() ? 1U : 0U);
  return {};
}

Expect<void> Executor::runRefFuncOp(Runtime::StackManager &StackMgr,
                                    uint32_t Idx) const noexcept {
  const auto *FuncInst = getFuncInstByIdx(StackMgr, Idx);
  StackMgr.push(RefVariant(FuncInst->getDefType(), FuncInst));
  return {};
}

Expect<void> Executor::runRefEqOp(ValVariant &Val1,
                                  const ValVariant &Val2) const noexcept {
  Val1.emplace<uint32_t>(Val1.get<RefVariant>().getPtr<void>() ==
                                 Val2.get<RefVariant>().getPtr<void>()
                             ? 1U
                             : 0U);
  return {};
}

Expect<void>
Executor::runRefAsNonNullOp(RefVariant &Ref,
                            const AST::Instruction &Instr) const noexcept {
  if (Ref.isNull()) {
    return Unexpect(logError(ErrCode::Value::CastNullToNonNull, Instr));
  }
  Ref.getType().toNonNullableRef();
  return {};
}

Expect<void> Executor::runStructNewOp(Runtime::StackManager &StackMgr,
                                      const uint32_t TypeIdx,
                                      const bool IsDefault) const noexcept {
  if (IsDefault) {
    StackMgr.push(*structNew(StackMgr, TypeIdx));
  } else {
    const auto &CompType = getCompositeTypeByIdx(StackMgr, TypeIdx);
    const uint32_t N = static_cast<uint32_t>(CompType.getFieldTypes().size());
    std::vector<ValVariant> Vals = StackMgr.pop(N);
    StackMgr.push(*structNew(StackMgr, TypeIdx, Vals));
  }
  return {};
}

Expect<void> Executor::runStructGetOp(Runtime::StackManager &StackMgr,
                                      const uint32_t TypeIdx,
                                      const uint32_t Off,
                                      const AST::Instruction &Instr,
                                      const bool IsSigned) const noexcept {
  const RefVariant Ref = StackMgr.getTop().get<RefVariant>();
  EXPECTED_TRY(
      auto Val,
      structGet(StackMgr, Ref, TypeIdx, Off, IsSigned).map_error([&](auto E) {
        return logError(E, Instr);
      }));
  StackMgr.getTop() = Val;
  return {};
}

Expect<void>
Executor::runStructSetOp(Runtime::StackManager &StackMgr, const ValVariant &Val,
                         const uint32_t TypeIdx, const uint32_t Off,
                         const AST::Instruction &Instr) const noexcept {
  const RefVariant Ref = StackMgr.pop().get<RefVariant>();
  EXPECTED_TRY(
      structSet(StackMgr, Ref, Val, TypeIdx, Off).map_error([&](auto E) {
        return logError(E, Instr);
      }));
  return {};
}

Expect<void> Executor::runArrayNewOp(Runtime::StackManager &StackMgr,
                                     const uint32_t TypeIdx,
                                     const uint32_t InitCnt,
                                     uint32_t Length) const noexcept {
  assuming(InitCnt == 0 || InitCnt == 1 || InitCnt == Length);
  if (InitCnt == 0) {
    StackMgr.push(*arrayNew(StackMgr, TypeIdx, Length));
  } else if (InitCnt == 1) {
    StackMgr.getTop().emplace<RefVariant>(
        *arrayNew(StackMgr, TypeIdx, Length, {StackMgr.getTop()}));
  } else {
    StackMgr.push(*arrayNew(StackMgr, TypeIdx, Length, StackMgr.pop(Length)));
  }
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
      arrayNewData(StackMgr, TypeIdx, DataIdx, Start, Length)
          .map_error([&](auto E) {
            auto *DataInst = getDataInstByIdx(StackMgr, DataIdx);
            const uint32_t BSize =
                getArrayStorageTypeByIdx(StackMgr, TypeIdx).getBitWidth() / 8;
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
  EXPECTED_TRY(auto InstRef,
               arrayNewElem(StackMgr, TypeIdx, ElemIdx, Start, Length)
                   .map_error([&](auto E) {
                     auto *ElemInst = getElemInstByIdx(StackMgr, ElemIdx);
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
  EXPECTED_TRY(
      auto Val,
      arrayGet(StackMgr, Ref, TypeIdx, Idx, IsSigned).map_error([&](auto E) {
        return logError(E, Instr,
                        [&]() { return logArrayOOB(E, Idx, 1, Ref); });
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
  EXPECTED_TRY(
      arraySet(StackMgr, Ref, Val, TypeIdx, Idx).map_error([&](auto E) {
        return logError(E, Instr,
                        [&]() { return logArrayOOB(E, Idx, 1, Ref); });
      }));
  return {};
}

Expect<void>
Executor::runArrayLenOp(ValVariant &Val,
                        const AST::Instruction &Instr) const noexcept {
  const auto *Inst =
      Val.get<RefVariant>().getPtr<Runtime::Instance::ArrayInstance>();
  if (Inst == nullptr) {
    return Unexpect(logError(ErrCode::Value::AccessNullArray, Instr));
  }
  Val.emplace<uint32_t>(Inst->getLength());
  return {};
}

Expect<void>
Executor::runArrayFillOp(Runtime::StackManager &StackMgr, const uint32_t Cnt,
                         const ValVariant &Val, const uint32_t TypeIdx,
                         const AST::Instruction &Instr) const noexcept {
  const uint32_t Idx = StackMgr.pop().get<uint32_t>();
  const RefVariant Ref = StackMgr.pop().get<RefVariant>();
  EXPECTED_TRY(
      arrayFill(StackMgr, Ref, Val, TypeIdx, Idx, Cnt).map_error([&](auto E) {
        return logError(E, Instr,
                        [&]() { return logArrayOOB(E, Idx, Cnt, Ref); });
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
  EXPECTED_TRY(arrayCopy(StackMgr, DstRef, DstTypeIdx, DstIdx, SrcRef,
                         SrcTypeIdx, SrcIdx, Cnt)
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
      arrayInitData(StackMgr, Ref, TypeIdx, DataIdx, DstIdx, SrcIdx, Cnt)
          .map_error([&](auto E) {
            auto *DataInst = getDataInstByIdx(StackMgr, DataIdx);
            const uint32_t BSize =
                getArrayStorageTypeByIdx(StackMgr, TypeIdx).getBitWidth() / 8;
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
      arrayInitElem(StackMgr, Ref, TypeIdx, ElemIdx, DstIdx, SrcIdx, Cnt)
          .map_error([&](auto E) {
            auto *ElemInst = getElemInstByIdx(StackMgr, ElemIdx);
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
  // Copy the value type here due to handling the externalized case.
  auto VT = Val.get<RefVariant>().getType();
  if (VT.isExternalized()) {
    VT = ValType(TypeCode::Ref, TypeCode::ExternRef);
  }
  Span<const AST::SubType *const> GotTypeList = ModInst->getTypeList();
  if (!VT.isAbsHeapType()) {
    auto *Inst =
        Val.get<RefVariant>().getPtr<Runtime::Instance::CompositeBase>();
    // Reference must not be nullptr here because the null references are typed
    // with the least abstract heap type.
    if (Inst->getModule()) {
      GotTypeList = Inst->getModule()->getTypeList();
    }
  }

  if (AST::TypeMatcher::matchType(ModInst->getTypeList(), Instr.getValType(),
                                  GotTypeList, VT)) {
    if (!IsCast) {
      Val.emplace<uint32_t>(1U);
    }
  } else {
    if (IsCast) {
      spdlog::error(ErrCode::Value::CastFailed);
      spdlog::error(ErrInfo::InfoMismatch(Instr.getValType(), VT));
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
  if (TCode == TypeCode::AnyRef) {
    // Internalize.
    if (Ref.isNull()) {
      Ref = RefVariant(ValType(TypeCode::RefNull, TypeCode::NullRef));
    } else {
      Ref.getType().setInternalized();
      if (Ref.getType().isExternRefType()) {
        Ref.getType() = ValType(TypeCode::Ref, TypeCode::AnyRef);
      }
    }
  } else {
    // Externalize.
    if (Ref.isNull()) {
      Ref = RefVariant(ValType(TypeCode::RefNull, TypeCode::NullExternRef));
    } else {
      // Use the externalize flag because the value type information should be
      // reserved when a reference being externalized and internalized.
      Ref.getType().setExternalized();
    }
  }
  return {};
}

Expect<void> Executor::runRefI31Op(ValVariant &Val) const noexcept {
  uint32_t RefNum = (Val.get<uint32_t>() & 0x7FFFFFFFU) | 0x80000000U;
  Val = RefVariant(ValType(TypeCode::Ref, TypeCode::I31Ref),
                   reinterpret_cast<void *>(static_cast<uint64_t>(RefNum)));
  return {};
}

Expect<void> Executor::runI31GetOp(ValVariant &Val,
                                   const AST::Instruction &Instr,
                                   const bool IsSigned) const noexcept {
  uint32_t RefNum = static_cast<uint32_t>(
      reinterpret_cast<uintptr_t>(Val.get<RefVariant>().getPtr<void>()));
  if ((RefNum & 0x80000000U) == 0) {
    return Unexpect(logError(ErrCode::Value::AccessNullI31, Instr));
  }
  RefNum &= 0x7FFFFFFFU;
  if (IsSigned) {
    RefNum |= ((RefNum & 0x40000000U) << 1);
  }
  Val.emplace<uint32_t>(RefNum);
  return {};
}

Expect<RefVariant>
Executor::structNew(Runtime::StackManager &StackMgr, const uint32_t TypeIdx,
                    Span<const ValVariant> Args) const noexcept {
  /// TODO: The array and struct instances are owned by the module instance
  /// currently because of referring the defined types of the module instances.
  /// This may be changed after applying the garbage collection mechanism.
  const auto &CompType = getCompositeTypeByIdx(StackMgr, TypeIdx);
  uint32_t N = static_cast<uint32_t>(CompType.getFieldTypes().size());
  Runtime::Instance::ModuleInstance *ModInst =
      const_cast<Runtime::Instance::ModuleInstance *>(StackMgr.getModule());
  std::vector<ValVariant> Vals(N);
  for (uint32_t I = 0; I < N; I++) {
    const auto &VType = CompType.getFieldTypes()[I].getStorageType();
    if (Args.size() > 0) {
      Vals[I] = packVal(VType, Args[I]);
    } else {
      Vals[I] = VType.isRefType()
                    ? ValVariant(RefVariant(toBottomType(StackMgr, VType)))
                    : ValVariant(static_cast<uint128_t>(0U));
    }
  }
  WasmEdge::Runtime::Instance::StructInstance *Inst =
      ModInst->newStruct(TypeIdx, std::move(Vals));
  return RefVariant(Inst->getDefType(), Inst);
}

Expect<ValVariant> Executor::structGet(Runtime::StackManager &StackMgr,
                                       const RefVariant Ref,
                                       const uint32_t TypeIdx,
                                       const uint32_t Off,
                                       const bool IsSigned) const noexcept {
  const auto *Inst = Ref.getPtr<Runtime::Instance::StructInstance>();
  if (Inst == nullptr) {
    return Unexpect(ErrCode::Value::AccessNullStruct);
  }
  const auto &VType = getStructStorageTypeByIdx(StackMgr, TypeIdx, Off);
  return unpackVal(VType, Inst->getField(Off), IsSigned);
}

Expect<void> Executor::structSet(Runtime::StackManager &StackMgr,
                                 const RefVariant Ref, const ValVariant Val,
                                 const uint32_t TypeIdx,
                                 const uint32_t Off) const noexcept {
  auto *Inst = Ref.getPtr<Runtime::Instance::StructInstance>();
  if (Inst == nullptr) {
    return Unexpect(ErrCode::Value::AccessNullStruct);
  }
  const auto &VType = getStructStorageTypeByIdx(StackMgr, TypeIdx, Off);
  Inst->getField(Off) = packVal(VType, Val);
  return {};
}

Expect<RefVariant>
Executor::arrayNew(Runtime::StackManager &StackMgr, const uint32_t TypeIdx,
                   const uint32_t Length,
                   Span<const ValVariant> Args) const noexcept {
  /// TODO: The array and struct instances are owned by the module instance
  /// currently because of referring the defined types of the module instances.
  /// This may be changed after applying the garbage collection mechanism.
  const auto &VType = getArrayStorageTypeByIdx(StackMgr, TypeIdx);
  WasmEdge::Runtime::Instance::ArrayInstance *Inst = nullptr;
  Runtime::Instance::ModuleInstance *ModInst =
      const_cast<Runtime::Instance::ModuleInstance *>(StackMgr.getModule());
  if (Args.size() == 0) {
    // New and fill with default values.
    auto InitVal = VType.isRefType()
                       ? ValVariant(RefVariant(toBottomType(StackMgr, VType)))
                       : ValVariant(static_cast<uint128_t>(0U));
    Inst = ModInst->newArray(TypeIdx, Length, InitVal);
  } else if (Args.size() == 1) {
    // New and fill with the arg value.
    Inst = ModInst->newArray(TypeIdx, Length, packVal(VType, Args[0]));
  } else {
    // New with args.
    Inst = ModInst->newArray(
        TypeIdx,
        packVals(VType, std::vector<ValVariant>(Args.begin(), Args.end())));
  }
  return RefVariant(Inst->getDefType(), Inst);
}

Expect<RefVariant>
Executor::arrayNewData(Runtime::StackManager &StackMgr, const uint32_t TypeIdx,
                       const uint32_t DataIdx, const uint32_t Start,
                       const uint32_t Length) const noexcept {
  const auto &VType = getArrayStorageTypeByIdx(StackMgr, TypeIdx);
  const uint32_t BSize = VType.getBitWidth() / 8;
  auto *DataInst = getDataInstByIdx(StackMgr, DataIdx);
  assuming(DataInst);
  if (static_cast<uint64_t>(Start) + static_cast<uint64_t>(Length) * BSize >
      DataInst->getData().size()) {
    return Unexpect(ErrCode::Value::MemoryOutOfBounds);
  }
  Runtime::Instance::ModuleInstance *ModInst =
      const_cast<Runtime::Instance::ModuleInstance *>(StackMgr.getModule());
  std::vector<ValVariant> Args;
  Args.reserve(Length);
  for (uint32_t Idx = 0; Idx < Length; Idx++) {
    // The value has been packed.
    Args.push_back(DataInst->loadValue(Start + Idx * BSize, BSize));
  }
  WasmEdge::Runtime::Instance::ArrayInstance *Inst =
      ModInst->newArray(TypeIdx, std::move(Args));
  return RefVariant(Inst->getDefType(), Inst);
}

Expect<RefVariant>
Executor::arrayNewElem(Runtime::StackManager &StackMgr, const uint32_t TypeIdx,
                       const uint32_t ElemIdx, const uint32_t Start,
                       const uint32_t Length) const noexcept {
  const auto &VType = getArrayStorageTypeByIdx(StackMgr, TypeIdx);
  auto *ElemInst = getElemInstByIdx(StackMgr, ElemIdx);
  assuming(ElemInst);
  auto ElemSrc = ElemInst->getRefs();
  if (static_cast<uint64_t>(Start) + static_cast<uint64_t>(Length) >
      ElemSrc.size()) {
    return Unexpect(ErrCode::Value::TableOutOfBounds);
  }
  std::vector<ValVariant> Refs(ElemSrc.begin() + Start,
                               ElemSrc.begin() + Start + Length);
  Runtime::Instance::ModuleInstance *ModInst =
      const_cast<Runtime::Instance::ModuleInstance *>(StackMgr.getModule());
  WasmEdge::Runtime::Instance::ArrayInstance *Inst =
      ModInst->newArray(TypeIdx, packVals(VType, std::move(Refs)));
  return RefVariant(Inst->getDefType(), Inst);
}

Expect<ValVariant> Executor::arrayGet(Runtime::StackManager &StackMgr,
                                      const RefVariant &Ref,
                                      const uint32_t TypeIdx,
                                      const uint32_t Idx,
                                      const bool IsSigned) const noexcept {
  const auto *Inst = Ref.getPtr<Runtime::Instance::ArrayInstance>();
  if (Inst == nullptr) {
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  if (Idx >= Inst->getLength()) {
    return Unexpect(ErrCode::Value::ArrayOutOfBounds);
  }
  const auto &VType = getArrayStorageTypeByIdx(StackMgr, TypeIdx);
  return unpackVal(VType, Inst->getData(Idx), IsSigned);
}

Expect<void> Executor::arraySet(Runtime::StackManager &StackMgr,
                                const RefVariant &Ref, const ValVariant &Val,
                                const uint32_t TypeIdx,
                                const uint32_t Idx) const noexcept {
  auto *Inst = Ref.getPtr<Runtime::Instance::ArrayInstance>();
  if (Inst == nullptr) {
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  if (Idx >= Inst->getLength()) {
    return Unexpect(ErrCode::Value::ArrayOutOfBounds);
  }
  const auto &VType = getArrayStorageTypeByIdx(StackMgr, TypeIdx);
  Inst->getData(Idx) = packVal(VType, Val);
  return {};
}

Expect<void> Executor::arrayFill(Runtime::StackManager &StackMgr,
                                 const RefVariant &Ref, const ValVariant &Val,
                                 const uint32_t TypeIdx, const uint32_t Idx,
                                 const uint32_t Cnt) const noexcept {
  auto *Inst = Ref.getPtr<Runtime::Instance::ArrayInstance>();
  if (Inst == nullptr) {
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  if (static_cast<uint64_t>(Idx) + static_cast<uint64_t>(Cnt) >
      Inst->getLength()) {
    return Unexpect(ErrCode::Value::ArrayOutOfBounds);
  }
  const auto &VType = getArrayStorageTypeByIdx(StackMgr, TypeIdx);
  auto Arr = Inst->getArray();
  std::fill(Arr.begin() + Idx, Arr.begin() + Idx + Cnt, packVal(VType, Val));
  return {};
}

Expect<void>
Executor::arrayInitData(Runtime::StackManager &StackMgr, const RefVariant &Ref,
                        const uint32_t TypeIdx, const uint32_t DataIdx,
                        const uint32_t DstIdx, const uint32_t SrcIdx,
                        const uint32_t Cnt) const noexcept {
  auto *Inst = Ref.getPtr<Runtime::Instance::ArrayInstance>();
  if (Inst == nullptr) {
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  if (static_cast<uint64_t>(DstIdx) + static_cast<uint64_t>(Cnt) >
      Inst->getLength()) {
    return Unexpect(ErrCode::Value::ArrayOutOfBounds);
  }
  const auto &VType = getArrayStorageTypeByIdx(StackMgr, TypeIdx);
  const uint32_t BSize = VType.getBitWidth() / 8;
  auto *DataInst = getDataInstByIdx(StackMgr, DataIdx);
  assuming(DataInst);
  if (static_cast<uint64_t>(SrcIdx) + static_cast<uint64_t>(Cnt) * BSize >
      DataInst->getData().size()) {
    return Unexpect(ErrCode::Value::MemoryOutOfBounds);
  }

  for (uint32_t Idx = 0; Idx < Cnt; Idx++) {
    // The value has been packed.
    Inst->getData(DstIdx + Idx) =
        DataInst->loadValue(SrcIdx + Idx * BSize, BSize);
  }
  return {};
}

Expect<void>
Executor::arrayInitElem(Runtime::StackManager &StackMgr, const RefVariant &Ref,
                        const uint32_t TypeIdx, const uint32_t ElemIdx,
                        const uint32_t DstIdx, const uint32_t SrcIdx,
                        const uint32_t Cnt) const noexcept {
  auto *Inst = Ref.getPtr<Runtime::Instance::ArrayInstance>();
  if (Inst == nullptr) {
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  if (static_cast<uint64_t>(DstIdx) + static_cast<uint64_t>(Cnt) >
      Inst->getLength()) {
    return Unexpect(ErrCode::Value::ArrayOutOfBounds);
  }
  const auto &VType = getArrayStorageTypeByIdx(StackMgr, TypeIdx);
  auto *ElemInst = getElemInstByIdx(StackMgr, ElemIdx);
  assuming(ElemInst);
  auto ElemSrc = ElemInst->getRefs();
  if (static_cast<uint64_t>(SrcIdx) + static_cast<uint64_t>(Cnt) >
      ElemSrc.size()) {
    return Unexpect(ErrCode::Value::TableOutOfBounds);
  }

  auto Arr = Inst->getArray();
  // The value has been packed.
  std::transform(ElemSrc.begin() + SrcIdx, ElemSrc.begin() + SrcIdx + Cnt,
                 Arr.begin() + DstIdx,
                 [&](const RefVariant &V) { return packVal(VType, V); });
  return {};
}

Expect<void>
Executor::arrayCopy(Runtime::StackManager &StackMgr, const RefVariant &DstRef,
                    const uint32_t DstTypeIdx, const uint32_t DstIdx,
                    const RefVariant &SrcRef, const uint32_t SrcTypeIdx,
                    const uint32_t SrcIdx, const uint32_t Cnt) const noexcept {
  auto *SrcInst = SrcRef.getPtr<Runtime::Instance::ArrayInstance>();
  auto *DstInst = DstRef.getPtr<Runtime::Instance::ArrayInstance>();
  if (SrcInst == nullptr) {
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  if (DstInst == nullptr) {
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  if (static_cast<uint64_t>(SrcIdx) + static_cast<uint64_t>(Cnt) >
      SrcInst->getLength()) {
    return Unexpect(ErrCode::Value::ArrayOutOfBounds);
  }
  if (static_cast<uint64_t>(DstIdx) + static_cast<uint64_t>(Cnt) >
      DstInst->getLength()) {
    return Unexpect(ErrCode::Value::ArrayOutOfBounds);
  }

  auto SrcArr = SrcInst->getArray();
  auto DstArr = DstInst->getArray();
  const auto &SrcVType = getArrayStorageTypeByIdx(StackMgr, SrcTypeIdx);
  const auto &DstVType = getArrayStorageTypeByIdx(StackMgr, DstTypeIdx);
  if (DstIdx <= SrcIdx) {
    std::transform(SrcArr.begin() + SrcIdx, SrcArr.begin() + SrcIdx + Cnt,
                   DstArr.begin() + DstIdx, [&](const ValVariant &V) {
                     return packVal(DstVType, unpackVal(SrcVType, V));
                   });
  } else {
    std::transform(std::make_reverse_iterator(SrcArr.begin() + SrcIdx + Cnt),
                   std::make_reverse_iterator(SrcArr.begin() + SrcIdx),
                   std::make_reverse_iterator(DstArr.begin() + DstIdx + Cnt),
                   [&](const ValVariant &V) {
                     return packVal(DstVType, unpackVal(SrcVType, V));
                   });
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
