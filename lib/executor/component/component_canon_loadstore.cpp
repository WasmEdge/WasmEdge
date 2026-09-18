// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {

namespace {
Expect<Runtime::Instance::MemoryInstance *>
getMemory(const Runtime::Component::CanonOptions &Opts) {
  if (unlikely(Opts.Mem == nullptr)) {
    spdlog::error(ErrCode::Value::InvalidCanonOption);
    spdlog::error("    the canonical option `memory` is required"sv);
    return Unexpect(ErrCode::Value::InvalidCanonOption);
  }
  return Opts.Mem;
}

/// Load an integer of the width of T, sign-extended when T is signed.
template <typename T>
Expect<T> loadInt(const Runtime::Component::CanonOptions &Opts, uint64_t Ptr) {
  EXPECTED_TRY(auto *Mem, getMemory(Opts));
  if constexpr (std::is_signed_v<T>) {
    int64_t Val = 0;
    EXPECTED_TRY((Mem->loadValue<int64_t, sizeof(T)>(Val, Ptr)));
    return static_cast<T>(Val);
  } else {
    uint64_t Val = 0;
    EXPECTED_TRY((Mem->loadValue<uint64_t, sizeof(T)>(Val, Ptr)));
    return static_cast<T>(Val);
  }
}

/// Store the low bytes of an integer at the width of T.
template <typename T>
Expect<void> storeInt(const Runtime::Component::CanonOptions &Opts,
                      uint64_t Ptr, T Val) {
  EXPECTED_TRY(auto *Mem, getMemory(Opts));
  const uint64_t Wide =
      static_cast<uint64_t>(static_cast<std::make_unsigned_t<T>>(Val));
  return Mem->storeValue<uint64_t, sizeof(T)>(Wide, Ptr);
}

Expect<uint32_t> loadDiscriminant(const Runtime::Component::CanonOptions &Opts,
                                  uint64_t Ptr, uint32_t Size) {
  switch (Size) {
  case 1:
    return loadInt<uint8_t>(Opts, Ptr);
  case 2:
    return loadInt<uint16_t>(Opts, Ptr);
  default:
    return loadInt<uint32_t>(Opts, Ptr);
  }
}

Expect<void> storeDiscriminant(const Runtime::Component::CanonOptions &Opts,
                               uint64_t Ptr, uint32_t Size, uint32_t Val) {
  switch (Size) {
  case 1:
    return storeInt<uint8_t>(Opts, Ptr, static_cast<uint8_t>(Val));
  case 2:
    return storeInt<uint16_t>(Opts, Ptr, static_cast<uint16_t>(Val));
  default:
    return storeInt<uint32_t>(Opts, Ptr, Val);
  }
}
} // namespace

// Check the alignment of a pointer. See
// "include/executor/component/executor.h".
Expect<void> ComponentExecutor::checkAligned(uint64_t Ptr, uint32_t Align) {
  if (Ptr % Align != 0) {
    spdlog::error(ErrCode::Value::ComponentPtrUnaligned);
    return Unexpect(ErrCode::Value::ComponentPtrUnaligned);
  }
  return {};
}

// Check a canonical byte length. See "include/executor/component/executor.h".
Expect<void> ComponentExecutor::checkByteLength(uint64_t Count, uint64_t Size) {
  if (Size != 0 && Count > MaxCopyLength / Size) {
    spdlog::error(ErrCode::Value::ComponentTrap);
    spdlog::error("    byte length exceeds the canonical ABI maximum"sv);
    return Unexpect(ErrCode::Value::ComponentTrap);
  }
  return {};
}

// Check a range of the memory. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::checkInBounds(const Runtime::Component::CanonOptions &Opts,
                                 uint64_t Ptr, uint64_t Len) {
  EXPECTED_TRY(auto *Mem, getMemory(Opts));
  if (!Mem->checkAccessBound(Ptr, Len)) {
    spdlog::error(ErrCode::Value::MemoryOutOfBounds);
    spdlog::error(ErrInfo::InfoBoundary(Ptr, Len, Mem->getSize()));
    return Unexpect(ErrCode::Value::MemoryOutOfBounds);
  }
  return {};
}

// Find the case a host value names. See
// "include/executor/component/executor.h".
Expect<uint32_t>
ComponentExecutor::findCaseIdx(uint32_t Case, std::string_view Label,
                               Span<const std::string> Labels) {
  if (!Label.empty()) {
    for (uint32_t I = 0; I < Labels.size(); ++I) {
      if (Labels[I] == Label) {
        return I;
      }
    }
    spdlog::error(ErrCode::Value::ComponentDiscriminantInvalid);
    spdlog::error("    invalid variant discriminant `{}`"sv, Label);
    return Unexpect(ErrCode::Value::ComponentDiscriminantInvalid);
  }
  if (Case >= Labels.size()) {
    spdlog::error(ErrCode::Value::ComponentDiscriminantInvalid);
    spdlog::error("    invalid variant discriminant {}"sv, Case);
    return Unexpect(ErrCode::Value::ComponentDiscriminantInvalid);
  }
  return Case;
}

// Load a pointer-sized integer. See "include/executor/component/executor.h".
Expect<uint64_t>
ComponentExecutor::loadPtr(const Runtime::Component::CanonOptions &Opts,
                           uint64_t Ptr) {
  if (Opts.isMemory64()) {
    return loadInt<uint64_t>(Opts, Ptr);
  }
  EXPECTED_TRY(const uint32_t Val, loadInt<uint32_t>(Opts, Ptr));
  return Val;
}

// Store a pointer-sized integer. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::storePtr(const Runtime::Component::CanonOptions &Opts,
                            uint64_t Ptr, uint64_t Val) {
  if (Opts.isMemory64()) {
    return storeInt<uint64_t>(Opts, Ptr, Val);
  }
  return storeInt<uint32_t>(Opts, Ptr, static_cast<uint32_t>(Val));
}

// Call realloc. See "include/executor/component/executor.h".
Expect<uint64_t>
ComponentExecutor::invokeRealloc(const Runtime::Component::CanonOptions &Opts,
                                 uint64_t OldPtr, uint64_t OldSize,
                                 uint32_t Align, uint64_t NewSize) {
  if (unlikely(Opts.Realloc == nullptr)) {
    spdlog::error(ErrCode::Value::InvalidCanonOption);
    spdlog::error("    the canonical option `realloc` is required"sv);
    return Unexpect(ErrCode::Value::InvalidCanonOption);
  }
  std::vector<ValVariant> Args;
  if (Opts.isMemory64()) {
    Args = {ValVariant(OldPtr), ValVariant(OldSize),
            ValVariant(static_cast<uint64_t>(Align)), ValVariant(NewSize)};
  } else {
    Args = {ValVariant(static_cast<uint32_t>(OldPtr)),
            ValVariant(static_cast<uint32_t>(OldSize)), ValVariant(Align),
            ValVariant(static_cast<uint32_t>(NewSize))};
  }
  EXPECTED_TRY(auto Rets, invokeCore(Opts.Realloc, Args));
  const uint64_t Ptr = Opts.isMemory64()
                           ? Rets[0].get<uint64_t>()
                           : static_cast<uint64_t>(Rets[0].get<uint32_t>());
  EXPECTED_TRY(checkAligned(Ptr, Align));
  EXPECTED_TRY(auto *Mem, getMemory(Opts));
  if (Ptr + NewSize > Mem->getSize()) {
    spdlog::error(ErrCode::Value::ComponentReallocOOB);
    return Unexpect(ErrCode::Value::ComponentReallocOOB);
  }
  return Ptr;
}

// Load a list. See "include/executor/component/executor.h".
Expect<ComponentValVariant> ComponentExecutor::loadList(
    const Runtime::Component::CanonOptions &Opts,
    const Runtime::Instance::ComponentInstance &TypeInst,
    const ComponentValType &ElemTy, uint64_t Ptr, uint64_t Len) {
  EXPECTED_TRY(const uint32_t Align,
               getAlignment(TypeInst, ElemTy, Opts.isMemory64()));
  EXPECTED_TRY(const uint64_t Size,
               getElemSize(TypeInst, ElemTy, Opts.isMemory64()));
  EXPECTED_TRY(checkByteLength(Len, Size));
  EXPECTED_TRY(checkAligned(Ptr, Align));
  EXPECTED_TRY(checkInBounds(Opts, Ptr, Len * Size));
  ListVal List;
  List.Elements.reserve(Len);
  for (uint64_t I = 0; I < Len; ++I) {
    EXPECTED_TRY(auto Elem, load(Opts, TypeInst, ElemTy, Ptr + I * Size));
    List.Elements.push_back(std::move(Elem));
  }
  return makeComponentVal(std::move(List));
}

// Load a list. See "include/executor/component/executor.h".
Expect<ComponentValVariant> ComponentExecutor::loadList(
    const Runtime::Component::CanonOptions &Opts,
    const Runtime::Instance::ComponentInstance &TypeInst,
    const AST::Component::DefValType &ElemTy, uint64_t Ptr, uint64_t Len) {
  EXPECTED_TRY(const uint32_t Align,
               getAlignment(TypeInst, ElemTy, Opts.isMemory64()));
  EXPECTED_TRY(const uint64_t Size,
               getElemSize(TypeInst, ElemTy, Opts.isMemory64()));
  EXPECTED_TRY(checkByteLength(Len, Size));
  EXPECTED_TRY(checkAligned(Ptr, Align));
  EXPECTED_TRY(checkInBounds(Opts, Ptr, Len * Size));
  ListVal List;
  List.Elements.reserve(Len);
  for (uint64_t I = 0; I < Len; ++I) {
    EXPECTED_TRY(auto Elem, load(Opts, TypeInst, ElemTy, Ptr + I * Size));
    List.Elements.push_back(std::move(Elem));
  }
  return makeComponentVal(std::move(List));
}

// Store a list. See "include/executor/component/executor.h".
Expect<std::pair<uint64_t, uint64_t>> ComponentExecutor::storeList(
    const Runtime::Component::CanonOptions &Opts,
    const Runtime::Instance::ComponentInstance &TypeInst,
    const ComponentValType &ElemTy, Span<const ComponentValVariant> Elems) {
  EXPECTED_TRY(const uint32_t Align,
               getAlignment(TypeInst, ElemTy, Opts.isMemory64()));
  EXPECTED_TRY(const uint64_t Size,
               getElemSize(TypeInst, ElemTy, Opts.isMemory64()));
  const uint64_t ByteLen = Elems.size() * Size;
  EXPECTED_TRY(const uint64_t Ptr, invokeRealloc(Opts, 0, 0, Align, ByteLen));
  for (size_t I = 0; I < Elems.size(); ++I) {
    EXPECTED_TRY(store(Opts, TypeInst, ElemTy, Elems[I], Ptr + I * Size));
  }
  return std::make_pair(Ptr, static_cast<uint64_t>(Elems.size()));
}

// Load a value. See "include/executor/component/executor.h".
Expect<ComponentValVariant>
ComponentExecutor::load(const Runtime::Component::CanonOptions &Opts,
                        const Runtime::Instance::ComponentInstance &TypeInst,
                        const ComponentValType &Ty, uint64_t Ptr) {
  if (!Ty.isPrimValType()) {
    EXPECTED_TRY(auto Resolved, getDefValType(TypeInst, Ty));
    return load(Opts, *Resolved.second, *Resolved.first, Ptr);
  }
  switch (Ty.getPrimValType()) {
  case PrimValType::Bool: {
    EXPECTED_TRY(const uint8_t Val, loadInt<uint8_t>(Opts, Ptr));
    return ComponentValVariant{Val != 0};
  }
  case PrimValType::S8: {
    EXPECTED_TRY(const int8_t Val, loadInt<int8_t>(Opts, Ptr));
    return ComponentValVariant{Val};
  }
  case PrimValType::U8: {
    EXPECTED_TRY(const uint8_t Val, loadInt<uint8_t>(Opts, Ptr));
    return ComponentValVariant{Val};
  }
  case PrimValType::S16: {
    EXPECTED_TRY(const int16_t Val, loadInt<int16_t>(Opts, Ptr));
    return ComponentValVariant{Val};
  }
  case PrimValType::U16: {
    EXPECTED_TRY(const uint16_t Val, loadInt<uint16_t>(Opts, Ptr));
    return ComponentValVariant{Val};
  }
  case PrimValType::S32: {
    EXPECTED_TRY(const int32_t Val, loadInt<int32_t>(Opts, Ptr));
    return ComponentValVariant{Val};
  }
  case PrimValType::U32: {
    EXPECTED_TRY(const uint32_t Val, loadInt<uint32_t>(Opts, Ptr));
    return ComponentValVariant{Val};
  }
  case PrimValType::S64: {
    EXPECTED_TRY(const int64_t Val, loadInt<int64_t>(Opts, Ptr));
    return ComponentValVariant{Val};
  }
  case PrimValType::U64: {
    EXPECTED_TRY(const uint64_t Val, loadInt<uint64_t>(Opts, Ptr));
    return ComponentValVariant{Val};
  }
  case PrimValType::F32: {
    EXPECTED_TRY(const uint32_t Bits, loadInt<uint32_t>(Opts, Ptr));
    float Val;
    std::memcpy(&Val, &Bits, sizeof(Val));
    return ComponentValVariant{canonicalizeNaN(Val)};
  }
  case PrimValType::F64: {
    EXPECTED_TRY(const uint64_t Bits, loadInt<uint64_t>(Opts, Ptr));
    double Val;
    std::memcpy(&Val, &Bits, sizeof(Val));
    return ComponentValVariant{canonicalizeNaN(Val)};
  }
  case PrimValType::Char: {
    EXPECTED_TRY(const uint32_t Val, loadInt<uint32_t>(Opts, Ptr));
    if (!isValidChar(Val)) {
      spdlog::error(ErrCode::Value::ComponentCharInvalid);
      return Unexpect(ErrCode::Value::ComponentCharInvalid);
    }
    return ComponentValVariant{Val};
  }
  case PrimValType::String: {
    const uint64_t PtrSize = Opts.isMemory64() ? 8 : 4;
    EXPECTED_TRY(const uint64_t StrPtr, loadPtr(Opts, Ptr));
    EXPECTED_TRY(const uint64_t PackedLen, loadPtr(Opts, Ptr + PtrSize));
    EXPECTED_TRY(auto Str, loadString(Opts, StrPtr, PackedLen));
    return ComponentValVariant{std::move(Str)};
  }
  case PrimValType::ErrorContext: {
    EXPECTED_TRY(const uint32_t Idx, loadInt<uint32_t>(Opts, Ptr));
    const auto *Msg =
        Opts.Inst != nullptr ? Opts.Inst->findErrorContext(Idx) : nullptr;
    if (Msg == nullptr) {
      spdlog::error(ErrCode::Value::ComponentHandleUnknown);
      spdlog::error("    unknown handle index {}"sv, Idx);
      return Unexpect(ErrCode::Value::ComponentHandleUnknown);
    }
    return makeComponentVal(ErrorContextVal{*Msg});
  }
  default:
    assumingUnreachable();
  }
}

// Load a value of a defined type. See
// "include/executor/component/executor.h".
Expect<ComponentValVariant>
ComponentExecutor::load(const Runtime::Component::CanonOptions &Opts,
                        const Runtime::Instance::ComponentInstance &TypeInst,
                        const AST::Component::DefValType &Ty, uint64_t Ptr) {
  const bool IsMem64 = Opts.isMemory64();
  if (Ty.isPrimValType()) {
    return load(Opts, TypeInst, ComponentValType(Ty.getPrimValType()), Ptr);
  }
  if (Ty.isRecordTy() || Ty.isTupleTy()) {
    // Fields at their aligned offsets.
    uint64_t Offset = 0;
    auto LoadField =
        [&](const ComponentValType &FieldTy) -> Expect<ComponentValVariant> {
      EXPECTED_TRY(const uint32_t FieldAlign,
                   getAlignment(TypeInst, FieldTy, IsMem64));
      EXPECTED_TRY(const uint64_t FieldSize,
                   getElemSize(TypeInst, FieldTy, IsMem64));
      Offset = alignTo(Offset, FieldAlign);
      EXPECTED_TRY(auto Val, load(Opts, TypeInst, FieldTy, Ptr + Offset));
      Offset += FieldSize;
      return Val;
    };
    if (Ty.isRecordTy()) {
      RecordVal Record;
      for (const auto &Field : Ty.getRecord().LabelTypes) {
        EXPECTED_TRY(auto Val, LoadField(Field.getValType()));
        Record.Fields.emplace_back(std::string(Field.getLabel()),
                                   std::move(Val));
      }
      return makeComponentVal(std::move(Record));
    }
    TupleVal Tuple;
    for (const auto &ElemTy : Ty.getTuple().Types) {
      EXPECTED_TRY(auto Val, LoadField(ElemTy));
      Tuple.Values.push_back(std::move(Val));
    }
    return makeComponentVal(std::move(Tuple));
  }
  if (Ty.isListTy()) {
    const auto &ListTy = Ty.getList();
    if (ListTy.Len.has_value()) {
      // A fixed-length list is inline.
      EXPECTED_TRY(const uint64_t ElemSize,
                   getElemSize(TypeInst, ListTy.ValTy, IsMem64));
      ListVal List;
      for (uint32_t I = 0; I < *ListTy.Len; ++I) {
        EXPECTED_TRY(auto Elem,
                     load(Opts, TypeInst, ListTy.ValTy, Ptr + I * ElemSize));
        List.Elements.push_back(std::move(Elem));
      }
      return makeComponentVal(std::move(List));
    }
    const uint64_t PtrSize = IsMem64 ? 8 : 4;
    EXPECTED_TRY(const uint64_t ElemPtr, loadPtr(Opts, Ptr));
    EXPECTED_TRY(const uint64_t Len, loadPtr(Opts, Ptr + PtrSize));
    return loadList(Opts, TypeInst, ListTy.ValTy, ElemPtr, Len);
  }
  if (Ty.isMapTy()) {
    // (map k v) is laid out as (list (tuple k v)).
    const uint64_t PtrSize = IsMem64 ? 8 : 4;
    EXPECTED_TRY(const uint64_t ElemPtr, loadPtr(Opts, Ptr));
    EXPECTED_TRY(const uint64_t Len, loadPtr(Opts, Ptr + PtrSize));
    AST::Component::DefValType EntryTy;
    EntryTy.setTuple(
        AST::Component::TupleTy{{Ty.getMap().KeyTy, Ty.getMap().ValTy}});
    return loadList(Opts, TypeInst, EntryTy, ElemPtr, Len);
  }
  if (Ty.isFlagsTy()) {
    const auto &Labels = Ty.getFlags().Labels;
    EXPECTED_TRY(const uint32_t Size, getAlignment(TypeInst, Ty, IsMem64));
    EXPECTED_TRY(const uint32_t Bits, loadDiscriminant(Opts, Ptr, Size));
    FlagsVal Flags;
    Flags.Bits.resize(Labels.size());
    for (size_t I = 0; I < Labels.size(); ++I) {
      Flags.Bits[I] = ((Bits >> I) & 1U) != 0;
      if (Flags.Bits[I]) {
        Flags.SetLabels.push_back(Labels[I]);
      }
    }
    return makeComponentVal(std::move(Flags));
  }
  if (Ty.isOwnTy() || Ty.isBorrowTy()) {
    EXPECTED_TRY(const uint32_t Handle, loadInt<uint32_t>(Opts, Ptr));
    if (Ty.isOwnTy()) {
      EXPECTED_TRY(const uint64_t Rep,
                   liftOwn(Opts, TypeInst, Ty.getOwn().Idx, Handle));
      return makeComponentVal(OwnVal{Rep});
    }
    EXPECTED_TRY(const uint64_t Rep,
                 liftBorrow(Opts, TypeInst, Ty.getBorrow().Idx, Handle));
    return makeComponentVal(BorrowVal{Rep});
  }
  if (Ty.isStreamTy() || Ty.isFutureTy()) {
    EXPECTED_TRY(const uint32_t Handle, loadInt<uint32_t>(Opts, Ptr));
    return liftStream(Opts, Handle, Ty.isStreamTy());
  }
  if (Ty.isVariantTy() || Ty.isOptionTy() || Ty.isResultTy() || Ty.isEnumTy()) {
    // The discriminant, then the payload of the case at the payload
    // alignment.
    const auto Cases = getVariantCases(Ty);
    const uint32_t DiscSize = getDiscriminantSize(Cases.size());
    EXPECTED_TRY(const uint32_t Case, loadDiscriminant(Opts, Ptr, DiscSize));
    if (Case >= Cases.size()) {
      spdlog::error(ErrCode::Value::ComponentDiscriminantInvalid);
      spdlog::error("    invalid variant discriminant {}"sv, Case);
      return Unexpect(ErrCode::Value::ComponentDiscriminantInvalid);
    }
    uint32_t MaxCaseAlign = 1;
    for (const auto &CaseTy : Cases) {
      if (CaseTy.has_value()) {
        EXPECTED_TRY(const uint32_t CaseAlign,
                     getAlignment(TypeInst, *CaseTy, IsMem64));
        MaxCaseAlign = std::max(MaxCaseAlign, CaseAlign);
      }
    }
    std::optional<ComponentValVariant> Payload;
    if (Cases[Case].has_value()) {
      EXPECTED_TRY(Payload, load(Opts, TypeInst, *Cases[Case],
                                 alignTo(Ptr + DiscSize, MaxCaseAlign)));
    }
    if (Ty.isOptionTy()) {
      return makeComponentVal(OptionVal{std::move(Payload)});
    }
    if (Ty.isResultTy()) {
      return makeComponentVal(ResultVal{Case == 0, std::move(Payload)});
    }
    if (Ty.isEnumTy()) {
      return makeComponentVal(EnumVal{Case, Ty.getEnum().Labels[Case]});
    }
    return makeComponentVal(VariantVal{Case, std::move(Payload),
                                       Ty.getVariant().Cases[Case].first});
  }
  spdlog::error(ErrCode::Value::InvalidTypeReference);
  spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefValType));
  return Unexpect(ErrCode::Value::InvalidTypeReference);
}

// Store a value. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::store(const Runtime::Component::CanonOptions &Opts,
                         const Runtime::Instance::ComponentInstance &TypeInst,
                         const ComponentValType &Ty,
                         const ComponentValVariant &Val, uint64_t Ptr) {
  if (!Ty.isPrimValType()) {
    EXPECTED_TRY(auto Resolved, getDefValType(TypeInst, Ty));
    return store(Opts, *Resolved.second, *Resolved.first, Val, Ptr);
  }
  switch (Ty.getPrimValType()) {
  case PrimValType::Bool:
    return storeInt<uint8_t>(Opts, Ptr, std::get<bool>(Val) ? 1 : 0);
  case PrimValType::S8:
    return storeInt<int8_t>(Opts, Ptr, std::get<int8_t>(Val));
  case PrimValType::U8:
    return storeInt<uint8_t>(Opts, Ptr, std::get<uint8_t>(Val));
  case PrimValType::S16:
    return storeInt<int16_t>(Opts, Ptr, std::get<int16_t>(Val));
  case PrimValType::U16:
    return storeInt<uint16_t>(Opts, Ptr, std::get<uint16_t>(Val));
  case PrimValType::S32:
    return storeInt<int32_t>(Opts, Ptr, std::get<int32_t>(Val));
  case PrimValType::U32:
  case PrimValType::Char:
    return storeInt<uint32_t>(Opts, Ptr, std::get<uint32_t>(Val));
  case PrimValType::S64:
    return storeInt<int64_t>(Opts, Ptr, std::get<int64_t>(Val));
  case PrimValType::U64:
    return storeInt<uint64_t>(Opts, Ptr, std::get<uint64_t>(Val));
  case PrimValType::F32: {
    uint32_t Bits;
    const float Float = canonicalizeNaN(std::get<float>(Val));
    std::memcpy(&Bits, &Float, sizeof(Bits));
    return storeInt<uint32_t>(Opts, Ptr, Bits);
  }
  case PrimValType::F64: {
    uint64_t Bits;
    const double Double = canonicalizeNaN(std::get<double>(Val));
    std::memcpy(&Bits, &Double, sizeof(Bits));
    return storeInt<uint64_t>(Opts, Ptr, Bits);
  }
  case PrimValType::String: {
    const uint64_t PtrSize = Opts.isMemory64() ? 8 : 4;
    EXPECTED_TRY(auto Range, storeString(Opts, std::get<std::string>(Val)));
    EXPECTED_TRY(storePtr(Opts, Ptr, Range.first));
    return storePtr(Opts, Ptr + PtrSize, Range.second);
  }
  case PrimValType::ErrorContext: {
    const auto &Msg = getComponentVal<ErrorContextVal>(Val).Message;
    const uint32_t Idx = Opts.Inst->addErrorContext(Msg);
    return storeInt<uint32_t>(Opts, Ptr, Idx);
  }
  default:
    assumingUnreachable();
  }
}

// Store a value of a defined type. See
// "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::store(const Runtime::Component::CanonOptions &Opts,
                         const Runtime::Instance::ComponentInstance &TypeInst,
                         const AST::Component::DefValType &Ty,
                         const ComponentValVariant &Val, uint64_t Ptr) {
  const bool IsMem64 = Opts.isMemory64();
  if (Ty.isPrimValType()) {
    return store(Opts, TypeInst, ComponentValType(Ty.getPrimValType()), Val,
                 Ptr);
  }
  if (Ty.isRecordTy() || Ty.isTupleTy()) {
    uint64_t Offset = 0;
    auto StoreField = [&](const ComponentValType &FieldTy,
                          const ComponentValVariant &FieldVal) -> Expect<void> {
      EXPECTED_TRY(const uint32_t FieldAlign,
                   getAlignment(TypeInst, FieldTy, IsMem64));
      EXPECTED_TRY(const uint64_t FieldSize,
                   getElemSize(TypeInst, FieldTy, IsMem64));
      Offset = alignTo(Offset, FieldAlign);
      EXPECTED_TRY(store(Opts, TypeInst, FieldTy, FieldVal, Ptr + Offset));
      Offset += FieldSize;
      return {};
    };
    if (Ty.isRecordTy()) {
      const auto &Record = getComponentVal<RecordVal>(Val);
      const auto &Types = Ty.getRecord().LabelTypes;
      for (size_t I = 0; I < Types.size(); ++I) {
        EXPECTED_TRY(
            StoreField(Types[I].getValType(), Record.Fields[I].second));
      }
      return {};
    }
    const auto &Tuple = getComponentVal<TupleVal>(Val);
    const auto &Types = Ty.getTuple().Types;
    for (size_t I = 0; I < Types.size(); ++I) {
      EXPECTED_TRY(StoreField(Types[I], Tuple.Values[I]));
    }
    return {};
  }
  if (Ty.isListTy()) {
    const auto &ListTy = Ty.getList();
    const auto &List = getComponentVal<ListVal>(Val);
    if (ListTy.Len.has_value()) {
      EXPECTED_TRY(const uint64_t ElemSize,
                   getElemSize(TypeInst, ListTy.ValTy, IsMem64));
      for (uint32_t I = 0; I < *ListTy.Len; ++I) {
        EXPECTED_TRY(store(Opts, TypeInst, ListTy.ValTy, List.Elements[I],
                           Ptr + I * ElemSize));
      }
      return {};
    }
    const uint64_t PtrSize = IsMem64 ? 8 : 4;
    EXPECTED_TRY(auto Range,
                 storeList(Opts, TypeInst, ListTy.ValTy, List.Elements));
    EXPECTED_TRY(storePtr(Opts, Ptr, Range.first));
    return storePtr(Opts, Ptr + PtrSize, Range.second);
  }
  if (Ty.isMapTy()) {
    const uint64_t PtrSize = IsMem64 ? 8 : 4;
    const auto &List = getComponentVal<ListVal>(Val);
    AST::Component::DefValType EntryTy;
    EntryTy.setTuple(
        AST::Component::TupleTy{{Ty.getMap().KeyTy, Ty.getMap().ValTy}});
    EXPECTED_TRY(const uint32_t Align,
                 getAlignment(TypeInst, EntryTy, IsMem64));
    EXPECTED_TRY(const uint64_t Size, getElemSize(TypeInst, EntryTy, IsMem64));
    EXPECTED_TRY(const uint64_t ElemPtr,
                 invokeRealloc(Opts, 0, 0, Align, List.Elements.size() * Size));
    for (size_t I = 0; I < List.Elements.size(); ++I) {
      EXPECTED_TRY(
          store(Opts, TypeInst, EntryTy, List.Elements[I], ElemPtr + I * Size));
    }
    EXPECTED_TRY(storePtr(Opts, Ptr, ElemPtr));
    return storePtr(Opts, Ptr + PtrSize, List.Elements.size());
  }
  if (Ty.isFlagsTy()) {
    const auto &Labels = Ty.getFlags().Labels;
    const auto &Flags = getComponentVal<FlagsVal>(Val);
    uint32_t Bits = 0;
    if (!Flags.SetLabels.empty()) {
      for (const auto &Set : Flags.SetLabels) {
        for (size_t I = 0; I < Labels.size(); ++I) {
          if (Labels[I] == Set) {
            Bits |= (1U << I);
          }
        }
      }
    } else {
      for (size_t I = 0; I < Flags.Bits.size() && I < Labels.size(); ++I) {
        if (Flags.Bits[I]) {
          Bits |= (1U << I);
        }
      }
    }
    EXPECTED_TRY(const uint32_t Size, getAlignment(TypeInst, Ty, IsMem64));
    return storeDiscriminant(Opts, Ptr, Size, Bits);
  }
  if (Ty.isOwnTy()) {
    EXPECTED_TRY(const uint32_t Handle,
                 lowerOwn(Opts, TypeInst, Ty.getOwn().Idx,
                          getComponentVal<OwnVal>(Val).Handle));
    return storeInt<uint32_t>(Opts, Ptr, Handle);
  }
  if (Ty.isBorrowTy()) {
    EXPECTED_TRY(const auto Slot,
                 lowerBorrow(Opts, TypeInst, Ty.getBorrow().Idx,
                             getComponentVal<BorrowVal>(Val).Handle));
    return storeInt<uint32_t>(Opts, Ptr, Slot.get<uint32_t>());
  }
  if (Ty.isStreamTy() || Ty.isFutureTy()) {
    EXPECTED_TRY(const uint32_t Handle, lowerStream(Opts, Val));
    return storeInt<uint32_t>(Opts, Ptr, Handle);
  }
  if (Ty.isVariantTy() || Ty.isOptionTy() || Ty.isResultTy() || Ty.isEnumTy()) {
    const auto Cases = getVariantCases(Ty);
    const uint32_t DiscSize = getDiscriminantSize(Cases.size());
    uint32_t Case = 0;
    const ComponentValVariant *Payload = nullptr;
    if (Ty.isOptionTy()) {
      const auto &Option = getComponentVal<OptionVal>(Val);
      Case = Option.Value.has_value() ? 1 : 0;
      Payload = Option.Value.has_value() ? &*Option.Value : nullptr;
    } else if (Ty.isResultTy()) {
      const auto &Res = getComponentVal<ResultVal>(Val);
      Case = Res.IsOk ? 0 : 1;
      Payload = Res.Payload.has_value() ? &*Res.Payload : nullptr;
    } else if (Ty.isEnumTy()) {
      const auto &Enum = getComponentVal<EnumVal>(Val);
      EXPECTED_TRY(Case,
                   findCaseIdx(Enum.Case, Enum.Label, Ty.getEnum().Labels));
    } else {
      const auto &Variant = getComponentVal<VariantVal>(Val);
      std::vector<std::string> Labels;
      for (const auto &[Label, PayloadTy] : Ty.getVariant().Cases) {
        Labels.push_back(Label);
      }
      EXPECTED_TRY(Case, findCaseIdx(Variant.Case, Variant.Label, Labels));
      Payload = Variant.Payload.has_value() ? &*Variant.Payload : nullptr;
    }
    EXPECTED_TRY(storeDiscriminant(Opts, Ptr, DiscSize, Case));
    if (Cases[Case].has_value() && Payload != nullptr) {
      uint32_t MaxCaseAlign = 1;
      for (const auto &CaseTy : Cases) {
        if (CaseTy.has_value()) {
          EXPECTED_TRY(const uint32_t CaseAlign,
                       getAlignment(TypeInst, *CaseTy, IsMem64));
          MaxCaseAlign = std::max(MaxCaseAlign, CaseAlign);
        }
      }
      return store(Opts, TypeInst, *Cases[Case], *Payload,
                   alignTo(Ptr + DiscSize, MaxCaseAlign));
    }
    return {};
  }
  spdlog::error(ErrCode::Value::InvalidTypeReference);
  spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefValType));
  return Unexpect(ErrCode::Value::InvalidTypeReference);
}

} // namespace Executor
} // namespace WasmEdge
