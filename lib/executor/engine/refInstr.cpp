// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"
#include "experimental/scope.hpp"

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

Expect<void>
Executor::runRefIsNullOp(Runtime::StackManager &StackMgr) const noexcept {
  const auto Val = StackMgr.peekTop<RefVariant>();
  StackMgr.emplaceTop<uint32_t>(Val.isNull() ? 1U : 0U);
  return {};
}

Expect<void> Executor::runRefFuncOp(Runtime::StackManager &StackMgr,
                                    uint32_t Idx) const noexcept {
  const auto *FuncInst = getFuncInstByIdx(StackMgr, Idx);
  StackMgr.push(RefVariant(FuncInst->getDefType(), FuncInst));
  return {};
}

Expect<void>
Executor::runRefEqOp(Runtime::StackManager &StackMgr) const noexcept {
  auto [Val2, Val1] = StackMgr.popsPeekTop<RefVariant, RefVariant>();
  StackMgr.emplaceTop<uint32_t>(
      Val1.getPtr<void>() == Val2.getPtr<void>() ? 1U : 0U);
  return {};
}

Expect<void>
Executor::runRefAsNonNullOp(Runtime::StackManager &StackMgr,
                            const AST::Instruction &Instr) const noexcept {
  auto Ref = StackMgr.peekTop<RefVariant>();
  if (Ref.isNull()) {
    return Unexpect(logError(ErrCode::Value::CastNullToNonNull, Instr));
  }
  Ref.getType().toNonNullableRef();
  StackMgr.emplaceTop(std::move(Ref));
  return {};
}

Expect<void> Executor::runStructNewOp(Runtime::StackManager &StackMgr,
                                      const uint32_t TypeIdx,
                                      const bool IsDefault) noexcept {
  Allocator.autoCollect();
  if (IsDefault) {
    EXPECTED_TRY(auto Ref, structNew(StackMgr, TypeIdx));
    StackMgr.push(std::move(Ref));
  } else {
    const auto &CompType = getCompositeTypeByIdx(StackMgr, TypeIdx);
    const uint32_t N = static_cast<uint32_t>(CompType.getFieldTypes().size());
    std::vector<ValVariant> Vals = StackMgr.popVec(N);
    EXPECTED_TRY(auto Ref, structNew(StackMgr, TypeIdx, Vals));
    StackMgr.push(std::move(Ref));
  }
  return {};
}

Expect<void> Executor::runStructGetOp(Runtime::StackManager &StackMgr,
                                      const uint32_t TypeIdx,
                                      const uint32_t Off,
                                      const AST::Instruction &Instr,
                                      const bool IsSigned) const noexcept {
  const auto Ref = StackMgr.peekTop<RefVariant>();
  EXPECTED_TRY(
      auto Val,
      structGet(StackMgr, Ref, TypeIdx, Off, IsSigned).map_error([&](auto E) {
        return logError(E, Instr);
      }));
  StackMgr.emplaceTop(std::move(Val));
  return {};
}

Expect<void>
Executor::runStructSetOp(Runtime::StackManager &StackMgr,
                         const uint32_t TypeIdx, const uint32_t Off,
                         const AST::Instruction &Instr) const noexcept {
  const auto [Val, Ref] = StackMgr.pops<ValVariant, RefVariant>();
  EXPECTED_TRY(
      structSet(StackMgr, Ref, Val, TypeIdx, Off).map_error([&](auto E) {
        return logError(E, Instr);
      }));
  return {};
}

Expect<void> Executor::runArrayNewOp(Runtime::StackManager &StackMgr,
                                     const uint32_t TypeIdx,
                                     const uint32_t InitCnt,
                                     uint32_t Length) noexcept {
  Allocator.autoCollect();
  assuming(InitCnt == 0 || InitCnt == 1 || InitCnt == Length);
  if (InitCnt == 0) {
    EXPECTED_TRY(auto Ref, arrayNew(StackMgr, TypeIdx, Length));
    StackMgr.push(std::move(Ref));
  } else if (InitCnt == 1) {
    const auto Val = StackMgr.peekTop<ValVariant>();
    EXPECTED_TRY(auto Ref, arrayNew(StackMgr, TypeIdx, Length, {Val}));
    StackMgr.emplaceTop(std::move(Ref));
  } else {
    const auto Vals = StackMgr.popVec(Length);
    EXPECTED_TRY(auto Ref, arrayNew(StackMgr, TypeIdx, Length, Vals));
    StackMgr.push(std::move(Ref));
  }
  return {};
}

Expect<void>
Executor::runArrayNewDataOp(Runtime::StackManager &StackMgr,
                            const uint32_t TypeIdx, const uint32_t DataIdx,
                            const AST::Instruction &Instr) noexcept {
  Allocator.autoCollect();
  const uint32_t Length = StackMgr.pop<uint32_t>();
  const uint32_t Start = StackMgr.peekTop<uint32_t>();
  EXPECTED_TRY(
      auto Ref,
      arrayNewData(StackMgr, TypeIdx, DataIdx, Start, Length)
          .map_error([&](auto E) {
            auto *DataInst = getDataInstByIdx(StackMgr, DataIdx);
            const uint32_t BSize =
                getArrayStorageTypeByIdx(StackMgr, TypeIdx).getBitWidth() / 8;
            return logError(E, Instr, [&]() {
              return logMemoryOOB(E, *DataInst, Start, BSize * Length);
            });
          }));
  StackMgr.emplaceTop<RefVariant>(std::move(Ref));
  return {};
}

Expect<void>
Executor::runArrayNewElemOp(Runtime::StackManager &StackMgr,
                            const uint32_t TypeIdx, const uint32_t ElemIdx,
                            const AST::Instruction &Instr) noexcept {
  Allocator.autoCollect();
  uint32_t Length, Start;
  std::tie(Length, Start) = StackMgr.popsPeekTop<uint32_t, uint32_t>();
  EXPECTED_TRY(auto Ref,
               arrayNewElem(StackMgr, TypeIdx, ElemIdx, Start, Length)
                   .map_error([&](auto E) {
                     auto *ElemInst = getElemInstByIdx(StackMgr, ElemIdx);
                     return logError(E, Instr, [&]() {
                       return logTableOOB(E, *ElemInst, Start, Length);
                     });
                   }));
  StackMgr.emplaceTop(std::move(Ref));
  return {};
}

Expect<void> Executor::runArrayGetOp(Runtime::StackManager &StackMgr,
                                     const uint32_t TypeIdx,
                                     const AST::Instruction &Instr,
                                     const bool IsSigned) const noexcept {
  uint32_t Idx;
  RefVariant Ref;
  std::tie(Idx, Ref) = StackMgr.popsPeekTop<uint32_t, RefVariant>();
  EXPECTED_TRY(
      auto Val,
      arrayGet(StackMgr, Ref, TypeIdx, Idx, IsSigned).map_error([&](auto E) {
        return logError(E, Instr,
                        [&]() { return logArrayOOB(E, Idx, 1, Ref); });
      }));
  StackMgr.emplaceTop(std::move(Val));
  return {};
}

Expect<void>
Executor::runArraySetOp(Runtime::StackManager &StackMgr, const uint32_t TypeIdx,
                        const AST::Instruction &Instr) const noexcept {
  ValVariant Val;
  uint32_t Idx;
  RefVariant Ref;
  std::tie(Val, Idx, Ref) = StackMgr.pops<ValVariant, uint32_t, RefVariant>();
  EXPECTED_TRY(
      arraySet(StackMgr, Ref, Val, TypeIdx, Idx).map_error([&](auto E) {
        return logError(E, Instr,
                        [&]() { return logArrayOOB(E, Idx, 1, Ref); });
      }));
  return {};
}

Expect<void>
Executor::runArrayLenOp(Runtime::StackManager &StackMgr,
                        const AST::Instruction &Instr) const noexcept {
  const auto Ref = StackMgr.peekTop<RefVariant>();
  auto *Raw = Ref.getPtr<Runtime::Instance::GCInstance::RawData>();
  if (Raw == nullptr) {
    spdlog::error(ErrCode::Value::AccessNullArray);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  const Runtime::Instance::ArrayInstance Inst{Raw};
  StackMgr.emplaceTop<uint32_t>(Inst.getLength());
  return {};
}

Expect<void>
Executor::runArrayFillOp(Runtime::StackManager &StackMgr,
                         const uint32_t TypeIdx,
                         const AST::Instruction &Instr) const noexcept {
  uint32_t Cnt;
  ValVariant Val;
  uint32_t Idx;
  RefVariant Ref;
  std::tie(Cnt, Val, Idx, Ref) =
      StackMgr.pops<uint32_t, ValVariant, uint32_t, RefVariant>();
  EXPECTED_TRY(
      arrayFill(StackMgr, Ref, Val, TypeIdx, Idx, Cnt).map_error([&](auto E) {
        return logError(E, Instr,
                        [&]() { return logArrayOOB(E, Idx, Cnt, Ref); });
      }));
  return {};
}

Expect<void>
Executor::runArrayCopyOp(Runtime::StackManager &StackMgr,
                         const uint32_t DstTypeIdx, const uint32_t SrcTypeIdx,
                         const AST::Instruction &Instr) const noexcept {
  uint32_t Cnt;
  uint32_t SrcIdx;
  RefVariant SrcRef;
  uint32_t DstIdx;
  RefVariant DstRef;
  std::tie(Cnt, SrcIdx, SrcRef, DstIdx, DstRef) =
      StackMgr.pops<uint32_t, uint32_t, RefVariant, uint32_t, RefVariant>();
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

Expect<void>
Executor::runArrayInitDataOp(Runtime::StackManager &StackMgr,
                             const uint32_t TypeIdx, const uint32_t DataIdx,
                             const AST::Instruction &Instr) const noexcept {
  uint32_t Cnt;
  uint32_t SrcIdx;
  uint32_t DstIdx;
  RefVariant Ref;
  std::tie(Cnt, SrcIdx, DstIdx, Ref) =
      StackMgr.pops<uint32_t, uint32_t, uint32_t, RefVariant>();
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

Expect<void>
Executor::runArrayInitElemOp(Runtime::StackManager &StackMgr,
                             const uint32_t TypeIdx, const uint32_t ElemIdx,
                             const AST::Instruction &Instr) const noexcept {
  uint32_t Cnt;
  uint32_t SrcIdx;
  uint32_t DstIdx;
  RefVariant Ref;
  std::tie(Cnt, SrcIdx, DstIdx, Ref) =
      StackMgr.pops<uint32_t, uint32_t, uint32_t, RefVariant>();
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

Expect<void> Executor::runRefTestOp(Runtime::StackManager &StackMgr,
                                    const AST::Instruction &Instr,
                                    bool IsCast) const noexcept {
  const Runtime::Instance::ModuleInstance *ModInst = StackMgr.getModule();
  const auto Ref = StackMgr.peekTop<RefVariant>();
  // Copy the value type here due to handling the externalized case.
  auto VT = Ref.getType();
  if (VT.isExternalized()) {
    VT = ValType(TypeCode::Ref, TypeCode::ExternRef);
  }
  Span<const AST::SubType *const> GotTypeList = ModInst->getTypeList();
  if (!VT.isAbsHeapType()) {
    auto *Raw = Ref.getPtr<Runtime::Instance::GCInstance::RawData>();
    // Reference must not be nullptr here because the null references are typed
    // with the least abstract heap type.
    if (Raw->ModInst) {
      GotTypeList = Raw->ModInst->getTypeList();
    }
  }

  if (AST::TypeMatcher::matchType(ModInst->getTypeList(), Instr.getValType(),
                                  GotTypeList, VT)) {
    if (!IsCast) {
      StackMgr.emplaceTop<uint32_t>(1U);
    }
  } else {
    if (IsCast) {
      spdlog::error(ErrCode::Value::CastFailed);
      spdlog::error(ErrInfo::InfoMismatch(Instr.getValType(), VT));
      spdlog::error(
          ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
      return Unexpect(ErrCode::Value::CastFailed);
    } else {
      StackMgr.emplaceTop<uint32_t>(0U);
    }
  }
  return {};
}

Expect<void> Executor::runRefConvOp(Runtime::StackManager &StackMgr,
                                    TypeCode TCode) const noexcept {
  auto Ref = StackMgr.peekTop<RefVariant>();
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
  StackMgr.emplaceTop(std::move(Ref));
  return {};
}

Expect<void>
Executor::runRefI31Op(Runtime::StackManager &StackMgr) const noexcept {
  uint32_t RefNum = (StackMgr.peekTop<uint32_t>() & 0x7FFFFFFFU) | 0x80000000U;
  StackMgr.emplaceTop(
      RefVariant(ValType(TypeCode::Ref, TypeCode::I31Ref),
                 reinterpret_cast<void *>(static_cast<uint64_t>(RefNum))));
  return {};
}

Expect<void> Executor::runI31GetOp(Runtime::StackManager &StackMgr,
                                   const AST::Instruction &Instr,
                                   const bool IsSigned) const noexcept {
  uint32_t RefNum = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(
      StackMgr.peekTop<RefVariant>().getPtr<void>()));
  if ((RefNum & 0x80000000U) == 0) {
    return Unexpect(logError(ErrCode::Value::AccessNullI31, Instr));
  }
  RefNum &= 0x7FFFFFFFU;
  if (IsSigned) {
    RefNum |= ((RefNum & 0x40000000U) << 1);
  }
  StackMgr.emplaceTop(std::move(RefNum));
  return {};
}

Expect<RefVariant> Executor::structNew(Runtime::StackManager &StackMgr,
                                       const uint32_t TypeIdx,
                                       Span<const ValVariant> Args) noexcept {
  /// TODO: The array and struct instances are owned by the module instance
  /// currently because of referring the defined types of the module instances.
  /// This may be changed after applying the garbage collection mechanism.
  const auto &CompType = getCompositeTypeByIdx(StackMgr, TypeIdx);
  uint32_t N = static_cast<uint32_t>(CompType.getFieldTypes().size());
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
  Runtime::Instance::StructInstance Inst(Allocator, StackMgr.getModule(),
                                         TypeIdx, std::move(Vals));
  if (Inst.getRaw() == nullptr) {
    return Unexpect(ErrCode::Value::AccessNullStruct);
  }
  return RefVariant(ValType(TypeCode::Ref, TypeIdx), Inst.getRaw());
}

Expect<ValVariant> Executor::structGet(Runtime::StackManager &StackMgr,
                                       const RefVariant Ref,
                                       const uint32_t TypeIdx,
                                       const uint32_t Off,
                                       const bool IsSigned) const noexcept {
  auto *Raw = Ref.getPtr<Runtime::Instance::GCInstance::RawData>();
  if (Raw == nullptr) {
    return Unexpect(ErrCode::Value::AccessNullStruct);
  }
  const Runtime::Instance::StructInstance Inst{Raw};
  const auto &VType = getStructStorageTypeByIdx(StackMgr, TypeIdx, Off);
  return unpackVal(VType, Inst.getField(Off), IsSigned);
}

Expect<void> Executor::structSet(Runtime::StackManager &StackMgr,
                                 const RefVariant Ref, const ValVariant Val,
                                 const uint32_t TypeIdx,
                                 const uint32_t Off) const noexcept {
  auto *Raw = Ref.getPtr<Runtime::Instance::GCInstance::RawData>();
  if (Raw == nullptr) {
    return Unexpect(ErrCode::Value::AccessNullStruct);
  }
  Runtime::Instance::StructInstance Inst{Raw};
  const auto &VType = getStructStorageTypeByIdx(StackMgr, TypeIdx, Off);
  Inst.getField(Off) = packVal(VType, Val);
  Allocator.writeBarrier(reinterpret_cast<uint8_t *>(Raw));
  return {};
}

Expect<RefVariant> Executor::arrayNew(Runtime::StackManager &StackMgr,
                                      const uint32_t TypeIdx,
                                      const uint32_t Length,
                                      Span<const ValVariant> Args) noexcept {
  /// TODO: The array and struct instances are owned by the module instance
  /// currently because of referring the defined types of the module instances.
  /// This may be changed after applying the garbage collection mechanism.
  Runtime::Instance::GCInstance::RawData *Raw = nullptr;
  const auto &VType = getArrayStorageTypeByIdx(StackMgr, TypeIdx);
  if (Args.size() == 0) {
    // New and fill with default values.
    auto InitVal = VType.isRefType()
                       ? ValVariant(RefVariant(toBottomType(StackMgr, VType)))
                       : ValVariant(static_cast<uint128_t>(0U));
    Runtime::Instance::ArrayInstance Inst(Allocator, StackMgr.getModule(),
                                          TypeIdx, Length, InitVal);
    Raw = Inst.getRaw();
  } else if (Args.size() == 1) {
    // New and fill with the arg value.
    Runtime::Instance::ArrayInstance Inst(Allocator, StackMgr.getModule(),
                                          TypeIdx, Length,
                                          packVal(VType, Args[0]));
    Raw = Inst.getRaw();
  } else {
    // New with args.
    Runtime::Instance::ArrayInstance Inst(
        Allocator, StackMgr.getModule(), TypeIdx,
        packVals(VType, std::vector<ValVariant>(Args.begin(), Args.end())));
    Raw = Inst.getRaw();
  }
  if (Raw == nullptr) {
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  return RefVariant(ValType(TypeCode::Ref, TypeIdx), Raw);
}

Expect<RefVariant> Executor::arrayNewData(Runtime::StackManager &StackMgr,
                                          const uint32_t TypeIdx,
                                          const uint32_t DataIdx,
                                          const uint32_t Start,
                                          const uint32_t Length) noexcept {
  const auto &VType = getArrayStorageTypeByIdx(StackMgr, TypeIdx);
  const uint32_t BSize = VType.getBitWidth() / 8;
  auto *DataInst = getDataInstByIdx(StackMgr, DataIdx);
  assuming(DataInst);
  if (static_cast<uint64_t>(Start) + static_cast<uint64_t>(Length) * BSize >
      DataInst->getData().size()) {
    return Unexpect(ErrCode::Value::MemoryOutOfBounds);
  }
  std::vector<ValVariant> Args;
  Args.reserve(Length);
  for (uint32_t Idx = 0; Idx < Length; Idx++) {
    // The value has been packed.
    Args.push_back(DataInst->loadValue(Start + Idx * BSize, BSize));
  }
  Runtime::Instance::ArrayInstance Inst(Allocator, StackMgr.getModule(),
                                        TypeIdx, std::move(Args));
  if (Inst.getRaw() == nullptr) {
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  return RefVariant(ValType(TypeCode::Ref, TypeIdx), Inst.getRaw());
}

Expect<RefVariant> Executor::arrayNewElem(Runtime::StackManager &StackMgr,
                                          const uint32_t TypeIdx,
                                          const uint32_t ElemIdx,
                                          const uint32_t Start,
                                          const uint32_t Length) noexcept {
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
  Runtime::Instance::ArrayInstance Inst(Allocator, StackMgr.getModule(),
                                        TypeIdx,
                                        packVals(VType, std::move(Refs)));
  if (Inst.getRaw() == nullptr) {
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  return RefVariant(ValType(TypeCode::Ref, TypeIdx), Inst.getRaw());
}

Expect<ValVariant> Executor::arrayGet(Runtime::StackManager &StackMgr,
                                      const RefVariant &Ref,
                                      const uint32_t TypeIdx,
                                      const uint32_t Idx,
                                      const bool IsSigned) const noexcept {
  auto *Raw = Ref.getPtr<Runtime::Instance::GCInstance::RawData>();
  if (Raw == nullptr) {
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  Runtime::Instance::ArrayInstance Inst{Raw};
  if (Idx >= Inst.getLength()) {
    return Unexpect(ErrCode::Value::ArrayOutOfBounds);
  }
  const auto &VType = getArrayStorageTypeByIdx(StackMgr, TypeIdx);
  return unpackVal(VType, Inst.getData(Idx), IsSigned);
}

Expect<void> Executor::arraySet(Runtime::StackManager &StackMgr,
                                const RefVariant &Ref, const ValVariant &Val,
                                const uint32_t TypeIdx,
                                const uint32_t Idx) const noexcept {
  auto *Raw = Ref.getPtr<Runtime::Instance::GCInstance::RawData>();
  if (Raw == nullptr) {
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  Runtime::Instance::ArrayInstance Inst{Raw};
  if (Idx >= Inst.getLength()) {
    return Unexpect(ErrCode::Value::ArrayOutOfBounds);
  }
  const auto &VType = getArrayStorageTypeByIdx(StackMgr, TypeIdx);
  Inst.getData(Idx) = packVal(VType, Val);
  Allocator.writeBarrier(reinterpret_cast<uint8_t *>(Raw));
  return {};
}

Expect<void> Executor::arrayFill(Runtime::StackManager &StackMgr,
                                 const RefVariant &Ref, const ValVariant &Val,
                                 const uint32_t TypeIdx, const uint32_t Idx,
                                 const uint32_t Cnt) const noexcept {
  auto *Raw = Ref.getPtr<Runtime::Instance::GCInstance::RawData>();
  if (Raw == nullptr) {
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  Runtime::Instance::ArrayInstance Inst{Raw};
  if (static_cast<uint64_t>(Idx) + static_cast<uint64_t>(Cnt) >
      Inst.getLength()) {
    return Unexpect(ErrCode::Value::ArrayOutOfBounds);
  }
  const auto &VType = getArrayStorageTypeByIdx(StackMgr, TypeIdx);
  auto Arr = Inst.getArray();
  std::fill(Arr.begin() + Idx, Arr.begin() + Idx + Cnt, packVal(VType, Val));
  Allocator.writeBarrier(reinterpret_cast<uint8_t *>(Raw));
  return {};
}

Expect<void>
Executor::arrayInitData(Runtime::StackManager &StackMgr, const RefVariant &Ref,
                        const uint32_t TypeIdx, const uint32_t DataIdx,
                        const uint32_t DstIdx, const uint32_t SrcIdx,
                        const uint32_t Cnt) const noexcept {
  auto *Raw = Ref.getPtr<Runtime::Instance::GCInstance::RawData>();
  if (Raw == nullptr) {
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  Runtime::Instance::ArrayInstance Inst{Raw};
  if (static_cast<uint64_t>(DstIdx) + static_cast<uint64_t>(Cnt) >
      Inst.getLength()) {
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
    Inst.getData(DstIdx + Idx) =
        DataInst->loadValue(SrcIdx + Idx * BSize, BSize);
  }
  Allocator.writeBarrier(reinterpret_cast<uint8_t *>(Raw));
  return {};
}

Expect<void>
Executor::arrayInitElem(Runtime::StackManager &StackMgr, const RefVariant &Ref,
                        const uint32_t TypeIdx, const uint32_t ElemIdx,
                        const uint32_t DstIdx, const uint32_t SrcIdx,
                        const uint32_t Cnt) const noexcept {
  auto *Raw = Ref.getPtr<Runtime::Instance::GCInstance::RawData>();
  if (Raw == nullptr) {
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  Runtime::Instance::ArrayInstance Inst{Raw};
  if (static_cast<uint64_t>(DstIdx) + static_cast<uint64_t>(Cnt) >
      Inst.getLength()) {
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

  auto Arr = Inst.getArray();
  // The value has been packed.
  std::transform(ElemSrc.begin() + SrcIdx, ElemSrc.begin() + SrcIdx + Cnt,
                 Arr.begin() + DstIdx,
                 [&](const RefVariant &V) { return packVal(VType, V); });
  Allocator.writeBarrier(reinterpret_cast<uint8_t *>(Raw));
  return {};
}

Expect<void>
Executor::arrayCopy(Runtime::StackManager &StackMgr, const RefVariant &DstRef,
                    const uint32_t DstTypeIdx, const uint32_t DstIdx,
                    const RefVariant &SrcRef, const uint32_t SrcTypeIdx,
                    const uint32_t SrcIdx, const uint32_t Cnt) const noexcept {
  auto *SrcRaw = SrcRef.getPtr<Runtime::Instance::GCInstance::RawData>();
  auto *DstRaw = DstRef.getPtr<Runtime::Instance::GCInstance::RawData>();
  if (SrcRaw == nullptr) {
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  if (DstRaw == nullptr) {
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  const Runtime::Instance::ArrayInstance SrcInst{SrcRaw};
  Runtime::Instance::ArrayInstance DstInst{DstRaw};
  if (static_cast<uint64_t>(SrcIdx) + static_cast<uint64_t>(Cnt) >
      SrcInst.getLength()) {
    return Unexpect(ErrCode::Value::ArrayOutOfBounds);
  }
  if (static_cast<uint64_t>(DstIdx) + static_cast<uint64_t>(Cnt) >
      DstInst.getLength()) {
    return Unexpect(ErrCode::Value::ArrayOutOfBounds);
  }

  auto SrcArr = SrcInst.getArray();
  auto DstArr = DstInst.getArray();
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
  Allocator.writeBarrier(reinterpret_cast<uint8_t *>(DstRaw));
  return {};
}

} // namespace Executor
} // namespace WasmEdge
