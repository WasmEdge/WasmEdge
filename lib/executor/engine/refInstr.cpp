// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

namespace {
ValVariant packVal(const ValType &Type, const ValVariant &Val) {
  if (Type.isPackType()) {
    switch (Type.getCode()) {
    case TypeCode::I8:
      return ValVariant(Val.get<uint32_t>() & 0xFFU);
    case TypeCode::I16:
      return ValVariant(Val.get<uint32_t>() & 0xFFFFU);
    default:
      assumingUnreachable();
    }
  }
  return Val;
}

ValVariant unpackVal(const ValType &Type, const ValVariant &Val,
                     bool IsSigned = false) {
  if (Type.isPackType()) {
    uint32_t Num = Val.get<uint32_t>();
    switch (Type.getCode()) {
    case TypeCode::I8:
      if (IsSigned) {
        return static_cast<uint32_t>(static_cast<int8_t>(Num));
      } else {
        return static_cast<uint32_t>(static_cast<uint8_t>(Num));
      }
    case TypeCode::I16:
      if (IsSigned) {
        return static_cast<uint32_t>(static_cast<int16_t>(Num));
      } else {
        return static_cast<uint32_t>(static_cast<uint16_t>(Num));
      }
    default:
      assumingUnreachable();
    }
  }
  return Val;
}

std::vector<ValVariant> packVals(const ValType &Type,
                                 std::vector<ValVariant> &&Vals) {
  for (uint32_t I = 0; I < Vals.size(); I++) {
    Vals[I] = packVal(Type, Vals[I]);
  }
  return std::move(Vals);
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
    spdlog::error(ErrCode::Value::CastNullToNonNull);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::CastNullToNonNull);
  }
  Ref.getType().toNonNullableRef();
  return {};
}

Expect<void> Executor::runStructNewOp(Runtime::StackManager &StackMgr,
                                      const uint32_t DefIndex,
                                      bool IsDefault) noexcept {
  const auto &CompType =
      getDefTypeByIdx(StackMgr, DefIndex)->getCompositeType();
  uint32_t N = static_cast<uint32_t>(CompType.getFieldTypes().size());
  std::vector<WasmEdge::ValVariant> Vals;
  if (IsDefault) {
    Vals.resize(N);
    for (uint32_t I = 0; I < N; I++) {
      const auto &SType = CompType.getFieldTypes()[I].getStorageType();
      Vals[I] = SType.isRefType()
                    ? ValVariant(RefVariant(toBottomType(StackMgr, SType)))
                    : ValVariant(static_cast<uint128_t>(0U));
    }
  } else {
    Vals = StackMgr.pop(N);
    for (uint32_t I = 0; I < N; I++) {
      Vals[I] = packVal(CompType.getFieldTypes()[I].getStorageType(), Vals[I]);
    }
  }
  Runtime::Instance::StructInstance Inst(Allocator, CompType, std::move(Vals));
  StackMgr.push(RefVariant(TypeCode::Struct, Inst.getRaw()));
  return {};
}

Expect<void> Executor::runStructGetOp(ValVariant &Val, const uint32_t Idx,
                                      const AST::CompositeType &CompType,
                                      const AST::Instruction &Instr,
                                      bool IsSigned) const noexcept {
  assuming(Val.get<RefVariant>().getType() == TypeCode::Struct);
  auto *Raw = Val.get<RefVariant>()
                  .getPtr<Runtime::Instance::StructInstance::RawStruct>();
  if (Raw == nullptr) {
    spdlog::error(ErrCode::Value::AccessNullStruct);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::AccessNullStruct);
  }
  const Runtime::Instance::StructInstance Inst{Raw};
  const auto &SType = CompType.getFieldTypes()[Idx].getStorageType();
  Val = unpackVal(SType, Inst.getField(Idx), IsSigned);
  return {};
}

Expect<void>
Executor::runStructSetOp(const ValVariant &Val, const RefVariant &InstRef,
                         const AST::CompositeType &CompType, uint32_t Idx,
                         const AST::Instruction &Instr) const noexcept {
  assuming(InstRef.getType() == TypeCode::Struct);
  auto *Raw = InstRef.getPtr<Runtime::Instance::StructInstance::RawStruct>();
  if (Raw == nullptr) {
    spdlog::error(ErrCode::Value::AccessNullStruct);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::AccessNullStruct);
  }
  Runtime::Instance::StructInstance Inst{Raw};
  const auto &SType = CompType.getFieldTypes()[Idx].getStorageType();
  Inst.getField(Idx) = packVal(SType, Val);
  return {};
}

Expect<void> Executor::runArrayNewOp(Runtime::StackManager &StackMgr,
                                     const uint32_t DefIndex, uint32_t InitCnt,
                                     uint32_t ValCnt) noexcept {
  assuming(InitCnt == 0 || InitCnt == 1 || InitCnt == ValCnt);
  const auto &CompType =
      getDefTypeByIdx(StackMgr, DefIndex)->getCompositeType();
  const auto &SType = CompType.getFieldTypes()[0].getStorageType();
  if (InitCnt == 0) {
    auto InitVal = SType.isRefType()
                       ? ValVariant(RefVariant(toBottomType(StackMgr, SType)))
                       : ValVariant(static_cast<uint128_t>(0U));
    Runtime::Instance::ArrayInstance Inst(Allocator, CompType, ValCnt, InitVal);
    StackMgr.push(RefVariant(TypeCode::Array, Inst.getRaw()));
  } else if (InitCnt == 1) {
    Runtime::Instance::ArrayInstance Inst(Allocator, CompType, ValCnt,
                                          packVal(SType, StackMgr.getTop()));
    StackMgr.getTop().emplace<RefVariant>(TypeCode::Array, Inst.getRaw());
  } else {
    Runtime::Instance::ArrayInstance Inst(
        Allocator, CompType, packVals(SType, StackMgr.pop(ValCnt)));
    StackMgr.push(RefVariant(TypeCode::Array, Inst.getRaw()));
  }
  return {};
}

Expect<void>
Executor::runArrayNewDataOp(Runtime::StackManager &StackMgr,
                            const Runtime::Instance::DataInstance &DataInst,
                            const AST::Instruction &Instr) noexcept {
  const uint32_t N = StackMgr.pop().get<uint32_t>();
  const uint32_t S = StackMgr.getTop().get<uint32_t>();
  const auto &CompType =
      getDefTypeByIdx(StackMgr, Instr.getTargetIndex())->getCompositeType();
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
  Runtime::Instance::ArrayInstance Inst(Allocator, CompType, N, 0U);
  for (uint32_t Idx = 0; Idx < N; Idx++) {
    // The value has been packed.
    Inst.getData(Idx) = DataInst.loadValue(S + Idx * BSize, BSize);
  }
  StackMgr.getTop().emplace<RefVariant>(TypeCode::Array, Inst.getRaw());
  return {};
}

Expect<void>
Executor::runArrayNewElemOp(Runtime::StackManager &StackMgr,
                            const Runtime::Instance::ElementInstance &ElemInst,
                            const AST::Instruction &Instr) noexcept {
  const uint32_t N = StackMgr.pop().get<uint32_t>();
  const uint32_t S = StackMgr.getTop().get<uint32_t>();
  const auto &CompType =
      getDefTypeByIdx(StackMgr, Instr.getTargetIndex())->getCompositeType();
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
  Runtime::Instance::ArrayInstance Inst(Allocator, CompType,
                                        packVals(SType, std::move(Refs)));
  StackMgr.getTop().emplace<RefVariant>(TypeCode::Array, Inst.getRaw());
  return {};
}

Expect<void>
Executor::runArraySetOp(const ValVariant &Val, const uint32_t Idx,
                        const RefVariant &InstRef,
                        const AST::CompositeType &CompType,
                        const AST::Instruction &Instr) const noexcept {
  assuming(InstRef.getType() == TypeCode::Array);
  auto *Raw = InstRef.getPtr<Runtime::Instance::ArrayInstance::RawArray>();
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
  Inst.getData(Idx) = packVal(SType, Val);
  return {};
}

Expect<void> Executor::runArrayGetOp(ValVariant &Val, const uint32_t Idx,
                                     const AST::CompositeType &CompType,
                                     const AST::Instruction &Instr,
                                     bool IsSigned) const noexcept {
  assuming(Val.get<RefVariant>().getType() == TypeCode::Array);
  auto *Raw = Val.get<RefVariant>()
                  .getPtr<Runtime::Instance::ArrayInstance::RawArray>();
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
  Val = unpackVal(SType, Inst.getData(Idx), IsSigned);
  return {};
}

Expect<void>
Executor::runArrayLenOp(ValVariant &Val,
                        const AST::Instruction &Instr) const noexcept {
  assuming(Val.get<RefVariant>().getType() == TypeCode::Array);
  auto *Raw = Val.get<RefVariant>()
                  .getPtr<Runtime::Instance::ArrayInstance::RawArray>();
  if (Raw == nullptr) {
    spdlog::error(ErrCode::Value::AccessNullArray);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  const Runtime::Instance::ArrayInstance Inst{Raw};
  Val.emplace<uint32_t>(Inst.getLength());
  return {};
}

Expect<void>
Executor::runArrayFillOp(uint32_t N, const ValVariant &Val, uint32_t D,
                         const RefVariant &InstRef,
                         const AST::CompositeType &CompType,
                         const AST::Instruction &Instr) const noexcept {
  assuming(InstRef.getType() == TypeCode::Array);
  auto *Raw = InstRef.getPtr<Runtime::Instance::ArrayInstance::RawArray>();
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
  return {};
}

Expect<void>
Executor::runArrayCopyOp(uint32_t N, uint32_t S, const RefVariant &SrcInstRef,
                         uint32_t D, const RefVariant &DstInstRef,
                         const AST::CompositeType &SrcCompType,
                         const AST::CompositeType &DstCompType,
                         const AST::Instruction &Instr) const noexcept {
  assuming(SrcInstRef.getType() == TypeCode::Array);
  assuming(DstInstRef.getType() == TypeCode::Array);
  auto *SrcRaw =
      SrcInstRef.getPtr<Runtime::Instance::ArrayInstance::RawArray>();
  auto *DstRaw =
      DstInstRef.getPtr<Runtime::Instance::ArrayInstance::RawArray>();
  if (SrcRaw == nullptr || DstRaw == nullptr) {
    spdlog::error(ErrCode::Value::AccessNullArray);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::AccessNullArray);
  }
  Runtime::Instance::ArrayInstance SrcInst{SrcRaw};
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
  return {};
}

Expect<void>
Executor::runArrayInitDataOp(uint32_t N, uint32_t S, uint32_t D,
                             const RefVariant &InstRef,
                             const AST::CompositeType &CompType,
                             const Runtime::Instance::DataInstance &DataInst,
                             const AST::Instruction &Instr) const noexcept {
  assuming(InstRef.getType() == TypeCode::Array);
  const uint32_t BSize =
      CompType.getFieldTypes()[0].getStorageType().getBitWidth() / 8;
  auto *Raw = InstRef.getPtr<Runtime::Instance::ArrayInstance::RawArray>();
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
  return {};
}

Expect<void>
Executor::runArrayInitElemOp(uint32_t N, uint32_t S, uint32_t D,
                             const RefVariant &InstRef,
                             const AST::CompositeType &CompType,
                             const Runtime::Instance::ElementInstance &ElemInst,
                             const AST::Instruction &Instr) const noexcept {
  assuming(InstRef.getType() == TypeCode::Array);
  auto ElemSrc = ElemInst.getRefs();
  auto *Raw = InstRef.getPtr<Runtime::Instance::ArrayInstance::RawArray>();
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
  return {};
}

Expect<void>
Executor::runRefTestOp(const Runtime::Instance::ModuleInstance *ModInst,
                       ValVariant &Val, const AST::Instruction &Instr,
                       bool IsCast) const noexcept {
  // Copy the value type here due to handling the externalized case.
  auto &VT = Val.get<RefVariant>().getType();
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
                                   bool IsSigned) const noexcept {
  uint32_t RefNum = static_cast<uint32_t>(
      reinterpret_cast<uintptr_t>(Val.get<RefVariant>().getPtr<void>()));
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
  Val.emplace<uint32_t>(RefNum);
  return {};
}

} // namespace Executor
} // namespace WasmEdge
