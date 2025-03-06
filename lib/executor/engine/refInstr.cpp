// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"
#include "experimental/scope.hpp"

namespace WasmEdge {
namespace Executor {

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
    spdlog::error(ErrCode::Value::CastNullToNonNull);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::CastNullToNonNull);
  }
  Ref.getType().toNonNullableRef();
  StackMgr.emplaceTop(std::move(Ref));
  return {};
}

Expect<void> Executor::runStructNewOp(Runtime::StackManager &StackMgr,
                                      const uint32_t DefIndex,
                                      bool IsDefault) noexcept {
  Allocator.collect();
  const auto &CompType =
      getDefTypeByIdx(StackMgr, DefIndex)->getCompositeType();
  uint32_t N = static_cast<uint32_t>(CompType.getFieldTypes().size());
  std::vector<ValVariant> Vals;
  if (IsDefault) {
    Vals.resize(N);
    for (uint32_t I = 0; I < N; I++) {
      const auto &SType = CompType.getFieldTypes()[I].getStorageType();
      Vals[I] = SType.isRefType()
                    ? ValVariant(RefVariant(toBottomType(StackMgr, SType)))
                    : ValVariant(static_cast<uint128_t>(0U));
    }
  } else {
    Vals = StackMgr.popVec(N);
    for (uint32_t I = 0; I < N; I++) {
      Vals[I] = packVal(CompType.getFieldTypes()[I].getStorageType(), Vals[I]);
    }
  }
  Runtime::Instance::StructInstance Inst(Allocator, StackMgr.getModule(),
                                         DefIndex, std::move(Vals));
  if (Inst.getRaw() == nullptr) {
    spdlog::error(ErrCode::Value::AccessNullStruct);
    return Unexpect(ErrCode::Value::AccessNullStruct);
  }
  StackMgr.push(RefVariant(ValType(TypeCode::Ref, DefIndex), Inst.getRaw()));
  return {};
}

Expect<void> Executor::runStructGetOp(Runtime::StackManager &StackMgr,
                                      const uint32_t Idx,
                                      const AST::CompositeType &CompType,
                                      const AST::Instruction &Instr,
                                      bool IsSigned) const noexcept {
  const auto Ref = StackMgr.peekTop<RefVariant>();
  auto *Raw = Ref.getPtr<Runtime::Instance::GCInstance::RawData>();
  if (Raw == nullptr) {
    spdlog::error(ErrCode::Value::AccessNullStruct);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::AccessNullStruct);
  }
  const Runtime::Instance::StructInstance Inst{Raw};
  const auto &SType = CompType.getFieldTypes()[Idx].getStorageType();
  StackMgr.emplaceTop(unpackVal(SType, Inst.getField(Idx), IsSigned));
  return {};
}

Expect<void>
Executor::runStructSetOp(Runtime::StackManager &StackMgr,
                         const AST::CompositeType &CompType, uint32_t Idx,
                         const AST::Instruction &Instr) const noexcept {
  const auto [Val, InstRef] = StackMgr.pops<ValVariant, RefVariant>();
  auto *Raw = InstRef.getPtr<Runtime::Instance::GCInstance::RawData>();
  if (Raw == nullptr) {
    spdlog::error(ErrCode::Value::AccessNullStruct);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::AccessNullStruct);
  }
  Runtime::Instance::StructInstance Inst{Raw};
  const auto &SType = CompType.getFieldTypes()[Idx].getStorageType();
  Inst.getField(Idx) = packVal(SType, Val);
  Allocator.writeBarrier(reinterpret_cast<uint8_t *>(Raw));
  return {};
}

Expect<void> Executor::runArrayNewOp(Runtime::StackManager &StackMgr,
                                     const uint32_t DefIndex, uint32_t InitCnt,
                                     uint32_t ValCnt) noexcept {
  Allocator.collect();
  assuming(InitCnt == 0 || InitCnt == 1 || InitCnt == ValCnt);
  const auto &CompType =
      getDefTypeByIdx(StackMgr, DefIndex)->getCompositeType();
  const auto &SType = CompType.getFieldTypes()[0].getStorageType();
  Runtime::Instance::GCInstance::RawData *Raw = nullptr;
  if (InitCnt == 0) {
    auto InitVal = SType.isRefType()
                       ? ValVariant(RefVariant(toBottomType(StackMgr, SType)))
                       : ValVariant(static_cast<uint128_t>(0U));
    Runtime::Instance::ArrayInstance Inst(Allocator, StackMgr.getModule(),
                                          DefIndex, ValCnt, InitVal);
    Raw = Inst.getRaw();
  } else if (InitCnt == 1) {
    Runtime::Instance::ArrayInstance Inst(
        Allocator, StackMgr.getModule(), DefIndex, ValCnt,
        packVal(SType, StackMgr.pop<ValVariant>()));
    Raw = Inst.getRaw();
  } else {
    Runtime::Instance::ArrayInstance Inst(
        Allocator, StackMgr.getModule(), DefIndex,
        packVals(SType, StackMgr.popVec(ValCnt)));
    Raw = Inst.getRaw();
  }
  if (Raw == nullptr) {
    spdlog::error(ErrCode::Value::AccessNullArray);
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  StackMgr.push(RefVariant(ValType(TypeCode::Ref, DefIndex), Raw));
  return {};
}

Expect<void>
Executor::runArrayNewDataOp(Runtime::StackManager &StackMgr,
                            const Runtime::Instance::DataInstance &DataInst,
                            const AST::Instruction &Instr) noexcept {
  Allocator.collect();
  const auto [N, S] = StackMgr.pops<uint32_t, uint32_t>();
  const uint32_t DefIndex = Instr.getTargetIndex();
  const auto &CompType =
      getDefTypeByIdx(StackMgr, DefIndex)->getCompositeType();
  const auto &SType = CompType.getFieldTypes()[0].getStorageType();
  const uint32_t BSize = SType.getBitWidth() / 8;
  if (static_cast<uint64_t>(S) + static_cast<uint64_t>(N) * BSize >
      DataInst.getData().size()) {
    spdlog::error(ErrCode::Value::MemoryOutOfBounds);
    spdlog::error(ErrInfo::InfoBoundary(
        static_cast<uint64_t>(S), N * BSize,
        DataInst.getData().size() > 0
            ? static_cast<uint32_t>(DataInst.getData().size() - 1)
            : 0U));
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::MemoryOutOfBounds);
  }
  Runtime::Instance::ArrayInstance Inst(Allocator, StackMgr.getModule(),
                                        DefIndex, N, 0U);
  if (Inst.getRaw() == nullptr) {
    spdlog::error(ErrCode::Value::AccessNullArray);
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  for (uint32_t Idx = 0; Idx < N; Idx++) {
    // The value has been packed.
    Inst.getData(Idx) = DataInst.loadValue(S + Idx * BSize, BSize);
  }
  StackMgr.push(RefVariant(ValType(TypeCode::Ref, DefIndex), Inst.getRaw()));
  return {};
}

Expect<void>
Executor::runArrayNewElemOp(Runtime::StackManager &StackMgr,
                            const Runtime::Instance::ElementInstance &ElemInst,
                            const AST::Instruction &Instr) noexcept {
  const auto [N, S] = StackMgr.pops<uint32_t, uint32_t>();
  const uint32_t DefIndex = Instr.getTargetIndex();
  const auto &CompType =
      getDefTypeByIdx(StackMgr, DefIndex)->getCompositeType();
  const auto &SType = CompType.getFieldTypes()[0].getStorageType();
  auto ElemSrc = ElemInst.getRefs();
  if (static_cast<uint64_t>(S) + static_cast<uint64_t>(N) > ElemSrc.size()) {
    spdlog::error(ErrCode::Value::TableOutOfBounds);
    spdlog::error(ErrInfo::InfoBoundary(
        static_cast<uint64_t>(S), N,
        ElemSrc.size() > 0 ? static_cast<uint32_t>(ElemSrc.size() - 1) : 0U));
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::TableOutOfBounds);
  }
  std::vector<ValVariant> Refs(ElemSrc.begin() + S, ElemSrc.begin() + S + N);
  Runtime::Instance::ArrayInstance Inst(Allocator, StackMgr.getModule(),
                                        DefIndex,
                                        packVals(SType, std::move(Refs)));
  if (Inst.getRaw() == nullptr) {
    spdlog::error(ErrCode::Value::AccessNullArray);
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  StackMgr.push(RefVariant(ValType(TypeCode::Ref, DefIndex), Inst.getRaw()));
  return {};
}

Expect<void>
Executor::runArraySetOp(Runtime::StackManager &StackMgr,
                        const AST::CompositeType &CompType,
                        const AST::Instruction &Instr) const noexcept {
  const auto [Val, Idx, InstRef] =
      StackMgr.pops<ValVariant, uint32_t, RefVariant>();
  auto *Raw = InstRef.getPtr<Runtime::Instance::GCInstance::RawData>();
  if (Raw == nullptr) {
    spdlog::error(ErrCode::Value::AccessNullArray);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  Runtime::Instance::ArrayInstance Inst{Raw};
  if (Idx >= Inst.getLength()) {
    spdlog::error(ErrCode::Value::ArrayOutOfBounds);
    spdlog::error(ErrInfo::InfoBoundary(Idx, 1, Inst.getBoundIdx()));
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::ArrayOutOfBounds);
  }
  const auto &SType = CompType.getFieldTypes()[0].getStorageType();
  ValVariant &Value = Inst.getData(Idx);
  Value = packVal(SType, Val);
  Allocator.writeBarrier(reinterpret_cast<uint8_t *>(Raw));
  return {};
}

Expect<void> Executor::runArrayGetOp(Runtime::StackManager &StackMgr,
                                     const AST::CompositeType &CompType,
                                     const AST::Instruction &Instr,
                                     bool IsSigned) const noexcept {
  const auto [Idx, Ref] = StackMgr.popsPeekTop<uint32_t, RefVariant>();
  auto *Raw = Ref.getPtr<Runtime::Instance::GCInstance::RawData>();
  if (Raw == nullptr) {
    spdlog::error(ErrCode::Value::AccessNullArray);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  const Runtime::Instance::ArrayInstance Inst{Raw};
  if (Idx >= Inst.getLength()) {
    spdlog::error(ErrCode::Value::ArrayOutOfBounds);
    spdlog::error(ErrInfo::InfoBoundary(Idx, 1, Inst.getBoundIdx()));
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::ArrayOutOfBounds);
  }
  const auto &SType = CompType.getFieldTypes()[0].getStorageType();
  StackMgr.emplaceTop(unpackVal(SType, Inst.getData(Idx), IsSigned));
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
                         const AST::CompositeType &CompType,
                         const AST::Instruction &Instr) const noexcept {
  const auto [N, Val, D, InstRef] =
      StackMgr.pops<uint32_t, ValVariant, uint32_t, RefVariant>();
  auto *Raw = InstRef.getPtr<Runtime::Instance::GCInstance::RawData>();
  if (Raw == nullptr) {
    spdlog::error(ErrCode::Value::AccessNullArray);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  Runtime::Instance::ArrayInstance Inst{Raw};
  if (static_cast<uint64_t>(D) + static_cast<uint64_t>(N) > Inst.getLength()) {
    spdlog::error(ErrCode::Value::ArrayOutOfBounds);
    spdlog::error(
        ErrInfo::InfoBoundary(static_cast<uint64_t>(D), N, Inst.getBoundIdx()));
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::ArrayOutOfBounds);
  }
  const auto &SType = CompType.getFieldTypes()[0].getStorageType();
  auto Arr = Inst.getArray();
  std::fill(Arr.begin() + D, Arr.begin() + D + N, packVal(SType, Val));
  Allocator.writeBarrier(reinterpret_cast<uint8_t *>(Raw));
  return {};
}

Expect<void>
Executor::runArrayCopyOp(Runtime::StackManager &StackMgr,
                         const AST::CompositeType &SrcCompType,
                         const AST::CompositeType &DstCompType,
                         const AST::Instruction &Instr) const noexcept {
  const auto [N, S, SrcInstRef, D, DstInstRef] =
      StackMgr.pops<uint32_t, uint32_t, RefVariant, uint32_t, RefVariant>();
  auto *SrcRaw = SrcInstRef.getPtr<Runtime::Instance::GCInstance::RawData>();
  auto *DstRaw = DstInstRef.getPtr<Runtime::Instance::GCInstance::RawData>();
  if (SrcRaw == nullptr || DstRaw == nullptr) {
    spdlog::error(ErrCode::Value::AccessNullArray);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  const Runtime::Instance::ArrayInstance SrcInst{SrcRaw};
  Runtime::Instance::ArrayInstance DstInst{DstRaw};
  if (static_cast<uint64_t>(S) + static_cast<uint64_t>(N) >
      SrcInst.getLength()) {
    spdlog::error(ErrCode::Value::ArrayOutOfBounds);
    spdlog::error(ErrInfo::InfoBoundary(static_cast<uint64_t>(S), N,
                                        SrcInst.getBoundIdx()));
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::ArrayOutOfBounds);
  }
  if (static_cast<uint64_t>(D) + static_cast<uint64_t>(N) >
      DstInst.getLength()) {
    spdlog::error(ErrCode::Value::ArrayOutOfBounds);
    spdlog::error(ErrInfo::InfoBoundary(static_cast<uint64_t>(D), N,
                                        DstInst.getBoundIdx()));
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::ArrayOutOfBounds);
  }
  const auto &SrcSType = SrcCompType.getFieldTypes()[0].getStorageType();
  const auto &DstSType = DstCompType.getFieldTypes()[0].getStorageType();
  auto SrcArr = SrcInst.getArray();
  auto DstArr = DstInst.getArray();
  if (D <= S) {
    std::transform(SrcArr.begin() + S, SrcArr.begin() + S + N,
                   DstArr.begin() + D, [&](const ValVariant &V) {
                     return packVal(DstSType, unpackVal(SrcSType, V));
                   });
  } else {
    std::transform(std::make_reverse_iterator(SrcArr.begin() + S + N),
                   std::make_reverse_iterator(SrcArr.begin() + S),
                   std::make_reverse_iterator(DstArr.begin() + D + N),
                   [&](const ValVariant &V) {
                     return packVal(DstSType, unpackVal(SrcSType, V));
                   });
  }
  Allocator.writeBarrier(reinterpret_cast<uint8_t *>(DstRaw));
  return {};
}

Expect<void>
Executor::runArrayInitDataOp(Runtime::StackManager &StackMgr,
                             const AST::CompositeType &CompType,
                             const Runtime::Instance::DataInstance &DataInst,
                             const AST::Instruction &Instr) const noexcept {
  const auto [N, S, D, InstRef] =
      StackMgr.pops<uint32_t, uint32_t, uint32_t, RefVariant>();
  const uint32_t BSize =
      CompType.getFieldTypes()[0].getStorageType().getBitWidth() / 8;
  auto *Raw = InstRef.getPtr<Runtime::Instance::GCInstance::RawData>();
  if (Raw == nullptr) {
    spdlog::error(ErrCode::Value::AccessNullArray);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  Runtime::Instance::ArrayInstance Inst{Raw};
  if (static_cast<uint64_t>(D) + static_cast<uint64_t>(N) > Inst.getLength()) {
    spdlog::error(ErrCode::Value::ArrayOutOfBounds);
    spdlog::error(
        ErrInfo::InfoBoundary(static_cast<uint64_t>(D), N, Inst.getBoundIdx()));
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::ArrayOutOfBounds);
  }
  if (static_cast<uint64_t>(S) + static_cast<uint64_t>(N) * BSize >
      DataInst.getData().size()) {
    spdlog::error(ErrCode::Value::MemoryOutOfBounds);
    spdlog::error(ErrInfo::InfoBoundary(
        static_cast<uint64_t>(S), N * BSize,
        DataInst.getData().size() > 0
            ? static_cast<uint32_t>(DataInst.getData().size() - 1)
            : 0U));
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::MemoryOutOfBounds);
  }
  for (uint32_t Off = 0; Off < N; Off++) {
    // The value has been packed.
    Inst.getData(D + Off) = DataInst.loadValue(S + Off * BSize, BSize);
  }
  Allocator.writeBarrier(reinterpret_cast<uint8_t *>(Raw));
  return {};
}

Expect<void>
Executor::runArrayInitElemOp(Runtime::StackManager &StackMgr,
                             const AST::CompositeType &CompType,
                             const Runtime::Instance::ElementInstance &ElemInst,
                             const AST::Instruction &Instr) const noexcept {
  const auto [N, S, D, InstRef] =
      StackMgr.pops<uint32_t, uint32_t, uint32_t, RefVariant>();
  auto ElemSrc = ElemInst.getRefs();
  auto *Raw = InstRef.getPtr<Runtime::Instance::GCInstance::RawData>();
  if (Raw == nullptr) {
    spdlog::error(ErrCode::Value::AccessNullArray);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  Runtime::Instance::ArrayInstance Inst{Raw};
  if (static_cast<uint64_t>(D) + static_cast<uint64_t>(N) > Inst.getLength()) {
    spdlog::error(ErrCode::Value::ArrayOutOfBounds);
    spdlog::error(
        ErrInfo::InfoBoundary(static_cast<uint64_t>(D), N, Inst.getBoundIdx()));
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::ArrayOutOfBounds);
  }
  if (static_cast<uint64_t>(S) + static_cast<uint64_t>(N) > ElemSrc.size()) {
    spdlog::error(ErrCode::Value::TableOutOfBounds);
    spdlog::error(ErrInfo::InfoBoundary(
        static_cast<uint64_t>(S), N,
        ElemSrc.size() > 0 ? static_cast<uint32_t>(ElemSrc.size() - 1) : 0U));
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::TableOutOfBounds);
  }
  const auto &SType = CompType.getFieldTypes()[0].getStorageType();

  auto Arr = Inst.getArray();
  // The value has been packed.
  std::transform(ElemSrc.begin() + S, ElemSrc.begin() + S + N, Arr.begin() + D,
                 [&](const RefVariant &V) { return packVal(SType, V); });
  Allocator.writeBarrier(reinterpret_cast<uint8_t *>(Raw));
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
                                   bool IsSigned) const noexcept {
  uint32_t RefNum = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(
      StackMgr.peekTop<RefVariant>().getPtr<void>()));
  if ((RefNum & 0x80000000U) == 0) {
    spdlog::error(ErrCode::Value::AccessNullI31);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::AccessNullI31);
  }
  RefNum &= 0x7FFFFFFFU;
  if (IsSigned) {
    RefNum |= ((RefNum & 0x40000000U) << 1);
  }
  StackMgr.emplaceTop(std::move(RefNum));
  return {};
}

} // namespace Executor
} // namespace WasmEdge
