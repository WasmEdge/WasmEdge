// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {

namespace {
Expect<void> logMatchError(std::string_view What) {
  spdlog::error(ErrCode::Value::FuncSigMismatch);
  spdlog::error("    host value does not match its type: {}"sv, What);
  return Unexpect(ErrCode::Value::FuncSigMismatch);
}

std::vector<std::string>
getVariantLabels(const AST::Component::DefValType &Ty) noexcept {
  std::vector<std::string> Labels;
  if (Ty.isVariantTy()) {
    for (const auto &[Label, Payload] : Ty.getVariant().Cases) {
      Labels.push_back(Label);
    }
  } else if (Ty.isEnumTy()) {
    Labels = Ty.getEnum().Labels;
  }
  return Labels;
}
/// A flat value as a bit pattern.
uint64_t getFlatBits(const ValVariant &Val, const ValType &Ty) noexcept {
  if (Ty.getCode() == TypeCode::I64 || Ty.getCode() == TypeCode::F64) {
    return Val.get<uint64_t>();
  }
  return Val.get<uint32_t>();
}

/// A flat value of type Ty from a bit pattern.
ValVariant newFlatValue(const ValType &Ty, uint64_t Bits) noexcept {
  if (Ty.getCode() == TypeCode::I64 || Ty.getCode() == TypeCode::F64) {
    return ValVariant(Bits);
  }
  return ValVariant(static_cast<uint32_t>(Bits));
}
} // namespace

// A canonical NaN. See "include/executor/component/executor.h".
float ComponentExecutor::canonicalizeNaN(float Val) const noexcept {
  if (!std::isnan(Val)) {
    return Val;
  }
  const uint32_t Bits = UINT32_C(0x7fc00000);
  std::memcpy(&Val, &Bits, sizeof(Val));
  return Val;
}

// A canonical NaN of double precision. See
// "include/executor/component/executor.h".
double ComponentExecutor::canonicalizeNaN(double Val) const noexcept {
  if (!std::isnan(Val)) {
    return Val;
  }
  const uint64_t Bits = UINT64_C(0x7ff8000000000000);
  std::memcpy(&Val, &Bits, sizeof(Val));
  return Val;
}

// Lift a value. See "include/executor/component/executor.h".
Expect<ComponentValVariant> ComponentExecutor::liftFlat(
    const Runtime::Component::CanonOptions &Opts,
    const Runtime::Instance::ComponentInstance &TypeInst,
    const ComponentValType &Ty, Span<const ValVariant> Flat, size_t &Pos) {
  if (!Ty.isPrimValType()) {
    EXPECTED_TRY(auto Resolved, getDefValType(TypeInst, Ty));
    return liftFlat(Opts, *Resolved.second, *Resolved.first, Flat, Pos);
  }
  auto PopFlat = [&Flat, &Pos]() -> const ValVariant & { return Flat[Pos++]; };
  switch (Ty.getPrimValType()) {
  case PrimValType::Bool:
    return ComponentValVariant{PopFlat().get<uint32_t>() != 0};
  case PrimValType::S8:
    return ComponentValVariant{static_cast<int8_t>(PopFlat().get<uint32_t>())};
  case PrimValType::U8:
    return ComponentValVariant{static_cast<uint8_t>(PopFlat().get<uint32_t>())};
  case PrimValType::S16:
    return ComponentValVariant{static_cast<int16_t>(PopFlat().get<uint32_t>())};
  case PrimValType::U16:
    return ComponentValVariant{
        static_cast<uint16_t>(PopFlat().get<uint32_t>())};
  case PrimValType::S32:
    return ComponentValVariant{static_cast<int32_t>(PopFlat().get<uint32_t>())};
  case PrimValType::U32:
    return ComponentValVariant{PopFlat().get<uint32_t>()};
  case PrimValType::S64:
    return ComponentValVariant{static_cast<int64_t>(PopFlat().get<uint64_t>())};
  case PrimValType::U64:
    return ComponentValVariant{PopFlat().get<uint64_t>()};
  case PrimValType::F32:
    return ComponentValVariant{canonicalizeNaN(PopFlat().get<float>())};
  case PrimValType::F64:
    return ComponentValVariant{canonicalizeNaN(PopFlat().get<double>())};
  case PrimValType::Char: {
    const uint32_t Val = PopFlat().get<uint32_t>();
    if (!isValidChar(Val)) {
      spdlog::error(ErrCode::Value::ComponentCharInvalid);
      return Unexpect(ErrCode::Value::ComponentCharInvalid);
    }
    return ComponentValVariant{Val};
  }
  case PrimValType::String: {
    const uint64_t Ptr = Opts.getFlatPtr(PopFlat());
    const uint64_t PackedLen = Opts.getFlatPtr(PopFlat());
    EXPECTED_TRY(auto Str, loadString(Opts, Ptr, PackedLen));
    return ComponentValVariant{std::move(Str)};
  }
  case PrimValType::ErrorContext: {
    const uint32_t Idx = PopFlat().get<uint32_t>();
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

// Lift a value of a defined type. See
// "include/executor/component/executor.h".
Expect<ComponentValVariant> ComponentExecutor::liftFlat(
    const Runtime::Component::CanonOptions &Opts,
    const Runtime::Instance::ComponentInstance &TypeInst,
    const AST::Component::DefValType &Ty, Span<const ValVariant> Flat,
    size_t &Pos) {
  const bool IsMem64 = Opts.isMemory64();
  auto PopFlat = [&Flat, &Pos]() -> const ValVariant & { return Flat[Pos++]; };
  if (Ty.isPrimValType()) {
    return liftFlat(Opts, TypeInst, ComponentValType(Ty.getPrimValType()), Flat,
                    Pos);
  }
  if (Ty.isRecordTy()) {
    RecordVal Record;
    for (const auto &Field : Ty.getRecord().LabelTypes) {
      EXPECTED_TRY(auto Val,
                   liftFlat(Opts, TypeInst, Field.getValType(), Flat, Pos));
      Record.Fields.emplace_back(std::string(Field.getLabel()), std::move(Val));
    }
    return makeComponentVal(std::move(Record));
  }
  if (Ty.isTupleTy()) {
    TupleVal Tuple;
    for (const auto &ElemTy : Ty.getTuple().Types) {
      EXPECTED_TRY(auto Val, liftFlat(Opts, TypeInst, ElemTy, Flat, Pos));
      Tuple.Values.push_back(std::move(Val));
    }
    return makeComponentVal(std::move(Tuple));
  }
  if (Ty.isListTy()) {
    const auto &ListTy = Ty.getList();
    if (ListTy.Len.has_value()) {
      ListVal List;
      for (uint32_t I = 0; I < *ListTy.Len; ++I) {
        EXPECTED_TRY(auto Elem,
                     liftFlat(Opts, TypeInst, ListTy.ValTy, Flat, Pos));
        List.Elements.push_back(std::move(Elem));
      }
      return makeComponentVal(std::move(List));
    }
    const uint64_t Ptr = Opts.getFlatPtr(PopFlat());
    const uint64_t Len = Opts.getFlatPtr(PopFlat());
    return loadList(Opts, TypeInst, ListTy.ValTy, Ptr, Len);
  }
  if (Ty.isMapTy()) {
    const uint64_t Ptr = Opts.getFlatPtr(PopFlat());
    const uint64_t Len = Opts.getFlatPtr(PopFlat());
    AST::Component::DefValType EntryTy;
    EntryTy.setTuple(
        AST::Component::TupleTy{{Ty.getMap().KeyTy, Ty.getMap().ValTy}});
    return loadList(Opts, TypeInst, EntryTy, Ptr, Len);
  }
  if (Ty.isFlagsTy()) {
    const auto &Labels = Ty.getFlags().Labels;
    const uint32_t Bits = PopFlat().get<uint32_t>();
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
  if (Ty.isOwnTy()) {
    EXPECTED_TRY(const uint64_t Rep, liftOwn(Opts, TypeInst, Ty.getOwn().Idx,
                                             PopFlat().get<uint32_t>()));
    return makeComponentVal(OwnVal{Rep});
  }
  if (Ty.isBorrowTy()) {
    EXPECTED_TRY(const uint64_t Rep,
                 liftBorrow(Opts, TypeInst, Ty.getBorrow().Idx,
                            PopFlat().get<uint32_t>()));
    return makeComponentVal(BorrowVal{Rep});
  }
  if (Ty.isStreamTy() || Ty.isFutureTy()) {
    return liftStream(Opts, PopFlat().get<uint32_t>(), Ty.isStreamTy());
  }
  if (Ty.isVariantTy() || Ty.isOptionTy() || Ty.isResultTy() || Ty.isEnumTy()) {
    // The discriminant, then the payload read through the joined slots.
    const auto Cases = getVariantCases(Ty);
    const uint32_t Case = PopFlat().get<uint32_t>();
    if (Case >= Cases.size()) {
      spdlog::error(ErrCode::Value::ComponentDiscriminantInvalid);
      spdlog::error("    invalid variant discriminant {}"sv, Case);
      return Unexpect(ErrCode::Value::ComponentDiscriminantInvalid);
    }
    std::vector<ValType> Joined;
    EXPECTED_TRY(flattenType(TypeInst, Ty, IsMem64, Joined));
    Joined.erase(Joined.begin());
    std::optional<ComponentValVariant> Payload;
    if (Cases[Case].has_value()) {
      std::vector<ValType> Have;
      EXPECTED_TRY(flattenType(TypeInst, *Cases[Case], IsMem64, Have));
      std::vector<ValVariant> Coerced;
      for (size_t I = 0; I < Have.size(); ++I) {
        Coerced.push_back(
            newFlatValue(Have[I], getFlatBits(Flat[Pos + I], Joined[I])));
      }
      size_t CasePos = 0;
      EXPECTED_TRY(Payload,
                   liftFlat(Opts, TypeInst, *Cases[Case], Coerced, CasePos));
    }
    Pos += Joined.size();
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
  spdlog::error(ErrCode::Value::ComponentTrap);
  spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefValType));
  return Unexpect(ErrCode::Value::ComponentTrap);
}

// Lower a value. See "include/executor/component/executor.h".
Expect<void> ComponentExecutor::lowerFlat(
    const Runtime::Component::CanonOptions &Opts,
    const Runtime::Instance::ComponentInstance &TypeInst,
    const ComponentValType &Ty, const ComponentValVariant &Val,
    std::vector<ValVariant> &Flat) {
  if (!Ty.isPrimValType()) {
    EXPECTED_TRY(auto Resolved, getDefValType(TypeInst, Ty));
    return lowerFlat(Opts, *Resolved.second, *Resolved.first, Val, Flat);
  }
  switch (Ty.getPrimValType()) {
  case PrimValType::Bool:
    Flat.emplace_back(static_cast<uint32_t>(std::get<bool>(Val) ? 1 : 0));
    return {};
  case PrimValType::S8:
    Flat.emplace_back(
        static_cast<uint32_t>(static_cast<int32_t>(std::get<int8_t>(Val))));
    return {};
  case PrimValType::U8:
    Flat.emplace_back(static_cast<uint32_t>(std::get<uint8_t>(Val)));
    return {};
  case PrimValType::S16:
    Flat.emplace_back(
        static_cast<uint32_t>(static_cast<int32_t>(std::get<int16_t>(Val))));
    return {};
  case PrimValType::U16:
    Flat.emplace_back(static_cast<uint32_t>(std::get<uint16_t>(Val)));
    return {};
  case PrimValType::S32:
    Flat.emplace_back(static_cast<uint32_t>(std::get<int32_t>(Val)));
    return {};
  case PrimValType::U32:
  case PrimValType::Char:
    Flat.emplace_back(std::get<uint32_t>(Val));
    return {};
  case PrimValType::S64:
    Flat.emplace_back(static_cast<uint64_t>(std::get<int64_t>(Val)));
    return {};
  case PrimValType::U64:
    Flat.emplace_back(std::get<uint64_t>(Val));
    return {};
  case PrimValType::F32:
    Flat.emplace_back(canonicalizeNaN(std::get<float>(Val)));
    return {};
  case PrimValType::F64:
    Flat.emplace_back(canonicalizeNaN(std::get<double>(Val)));
    return {};
  case PrimValType::String: {
    EXPECTED_TRY(auto Range, storeString(Opts, std::get<std::string>(Val)));
    Flat.push_back(Opts.newFlatPtr(Range.first));
    Flat.push_back(Opts.newFlatPtr(Range.second));
    return {};
  }
  case PrimValType::ErrorContext: {
    const auto &Msg = getComponentVal<ErrorContextVal>(Val).Message;
    Flat.emplace_back(Opts.Inst->addErrorContext(Msg));
    return {};
  }
  default:
    assumingUnreachable();
  }
}

// Lower a value of a defined type. See
// "include/executor/component/executor.h".
Expect<void> ComponentExecutor::lowerFlat(
    const Runtime::Component::CanonOptions &Opts,
    const Runtime::Instance::ComponentInstance &TypeInst,
    const AST::Component::DefValType &Ty, const ComponentValVariant &Val,
    std::vector<ValVariant> &Flat) {
  const bool IsMem64 = Opts.isMemory64();
  if (Ty.isPrimValType()) {
    return lowerFlat(Opts, TypeInst, ComponentValType(Ty.getPrimValType()), Val,
                     Flat);
  }
  if (Ty.isRecordTy()) {
    const auto &Record = getComponentVal<RecordVal>(Val);
    const auto &Types = Ty.getRecord().LabelTypes;
    for (size_t I = 0; I < Types.size(); ++I) {
      EXPECTED_TRY(lowerFlat(Opts, TypeInst, Types[I].getValType(),
                             Record.Fields[I].second, Flat));
    }
    return {};
  }
  if (Ty.isTupleTy()) {
    const auto &Tuple = getComponentVal<TupleVal>(Val);
    const auto &Types = Ty.getTuple().Types;
    for (size_t I = 0; I < Types.size(); ++I) {
      EXPECTED_TRY(lowerFlat(Opts, TypeInst, Types[I], Tuple.Values[I], Flat));
    }
    return {};
  }
  if (Ty.isListTy()) {
    const auto &ListTy = Ty.getList();
    const auto &List = getComponentVal<ListVal>(Val);
    if (ListTy.Len.has_value()) {
      for (uint32_t I = 0; I < *ListTy.Len; ++I) {
        EXPECTED_TRY(
            lowerFlat(Opts, TypeInst, ListTy.ValTy, List.Elements[I], Flat));
      }
      return {};
    }
    EXPECTED_TRY(auto Range,
                 storeList(Opts, TypeInst, ListTy.ValTy, List.Elements));
    Flat.push_back(Opts.newFlatPtr(Range.first));
    Flat.push_back(Opts.newFlatPtr(Range.second));
    return {};
  }
  if (Ty.isMapTy()) {
    const auto &List = getComponentVal<ListVal>(Val);
    AST::Component::DefValType EntryTy;
    EntryTy.setTuple(
        AST::Component::TupleTy{{Ty.getMap().KeyTy, Ty.getMap().ValTy}});
    EXPECTED_TRY(const uint32_t Align,
                 getAlignment(TypeInst, EntryTy, IsMem64));
    EXPECTED_TRY(const uint64_t Size, getElemSize(TypeInst, EntryTy, IsMem64));
    EXPECTED_TRY(const uint64_t Ptr,
                 invokeRealloc(Opts, 0, 0, Align, List.Elements.size() * Size));
    for (size_t I = 0; I < List.Elements.size(); ++I) {
      EXPECTED_TRY(
          store(Opts, TypeInst, EntryTy, List.Elements[I], Ptr + I * Size));
    }
    Flat.push_back(Opts.newFlatPtr(Ptr));
    Flat.push_back(Opts.newFlatPtr(List.Elements.size()));
    return {};
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
    Flat.emplace_back(Bits);
    return {};
  }
  if (Ty.isOwnTy()) {
    EXPECTED_TRY(const uint32_t Handle,
                 lowerOwn(Opts, TypeInst, Ty.getOwn().Idx,
                          getComponentVal<OwnVal>(Val).Handle));
    Flat.emplace_back(Handle);
    return {};
  }
  if (Ty.isBorrowTy()) {
    EXPECTED_TRY(auto Slot,
                 lowerBorrow(Opts, TypeInst, Ty.getBorrow().Idx,
                             getComponentVal<BorrowVal>(Val).Handle));
    Flat.push_back(Slot);
    return {};
  }
  if (Ty.isStreamTy() || Ty.isFutureTy()) {
    EXPECTED_TRY(const uint32_t Handle, lowerStream(Opts, Val));
    Flat.emplace_back(Handle);
    return {};
  }
  if (Ty.isVariantTy() || Ty.isOptionTy() || Ty.isResultTy() || Ty.isEnumTy()) {
    // The discriminant, then the payload coerced into the joined slots and
    // padded with zeros.
    const auto Cases = getVariantCases(Ty);
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
                   findCaseIdx(Enum.Case, Enum.Label, getVariantLabels(Ty)));
    } else {
      const auto &Variant = getComponentVal<VariantVal>(Val);
      EXPECTED_TRY(
          Case, findCaseIdx(Variant.Case, Variant.Label, getVariantLabels(Ty)));
      Payload = Variant.Payload.has_value() ? &*Variant.Payload : nullptr;
    }
    std::vector<ValType> Joined;
    EXPECTED_TRY(flattenType(TypeInst, Ty, IsMem64, Joined));
    Joined.erase(Joined.begin());
    Flat.emplace_back(Case);
    std::vector<ValVariant> PayloadFlat;
    std::vector<ValType> Have;
    if (Cases[Case].has_value() && Payload != nullptr) {
      EXPECTED_TRY(flattenType(TypeInst, *Cases[Case], IsMem64, Have));
      EXPECTED_TRY(
          lowerFlat(Opts, TypeInst, *Cases[Case], *Payload, PayloadFlat));
    }
    for (size_t I = 0; I < Joined.size(); ++I) {
      if (I < PayloadFlat.size()) {
        Flat.push_back(
            newFlatValue(Joined[I], getFlatBits(PayloadFlat[I], Have[I])));
      } else {
        Flat.push_back(newFlatValue(Joined[I], 0));
      }
    }
    return {};
  }
  spdlog::error(ErrCode::Value::ComponentTrap);
  spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefValType));
  return Unexpect(ErrCode::Value::ComponentTrap);
}

// Lift a value list. See "include/executor/component/executor.h".
Expect<std::vector<ComponentValVariant>> ComponentExecutor::liftFlatValues(
    const Runtime::Component::CanonOptions &Opts,
    const Runtime::Instance::ComponentInstance &TypeInst,
    Span<const ComponentValType> Types, Span<const ValVariant> Flat,
    uint32_t MaxFlat) {
  const bool IsMem64 = Opts.isMemory64();
  std::vector<ValType> FlatTypes;
  for (const auto &Ty : Types) {
    EXPECTED_TRY(flattenType(TypeInst, Ty, IsMem64, FlatTypes));
  }
  std::vector<ComponentValVariant> Vals;
  Vals.reserve(Types.size());
  if (FlatTypes.size() > MaxFlat) {
    // Past the limit the values sit in memory as a tuple at the one pointer.
    const uint64_t Ptr = Opts.getFlatPtr(Flat[0]);
    uint64_t Offset = 0;
    uint32_t TupleAlign = 1;
    for (const auto &Ty : Types) {
      EXPECTED_TRY(const uint32_t Align, getAlignment(TypeInst, Ty, IsMem64));
      TupleAlign = std::max(TupleAlign, Align);
    }
    EXPECTED_TRY(checkAligned(Ptr, TupleAlign));
    for (const auto &Ty : Types) {
      EXPECTED_TRY(const uint32_t Align, getAlignment(TypeInst, Ty, IsMem64));
      EXPECTED_TRY(const uint64_t Size, getElemSize(TypeInst, Ty, IsMem64));
      Offset = alignTo(Offset, Align);
      EXPECTED_TRY(auto Val, load(Opts, TypeInst, Ty, Ptr + Offset));
      Vals.push_back(std::move(Val));
      Offset += Size;
    }
    return Vals;
  }
  size_t Pos = 0;
  for (const auto &Ty : Types) {
    EXPECTED_TRY(auto Val, liftFlat(Opts, TypeInst, Ty, Flat, Pos));
    Vals.push_back(std::move(Val));
  }
  assuming(Pos == Flat.size());
  return Vals;
}

// Lower a value list. See "include/executor/component/executor.h".
Expect<void> ComponentExecutor::lowerFlatValues(
    const Runtime::Component::CanonOptions &Opts,
    const Runtime::Instance::ComponentInstance &TypeInst,
    Span<const ComponentValType> Types, Span<const ComponentValVariant> Vals,
    uint32_t MaxFlat, std::vector<ValVariant> &Flat,
    std::optional<uint64_t> OutPtr) {
  const bool IsMem64 = Opts.isMemory64();
  if (Vals.size() != Types.size()) {
    return logMatchError("value count"sv);
  }
  for (size_t I = 0; I < Types.size(); ++I) {
    EXPECTED_TRY(checkHostValue(TypeInst, Types[I], Vals[I]));
  }
  std::vector<ValType> FlatTypes;
  for (const auto &Ty : Types) {
    EXPECTED_TRY(flattenType(TypeInst, Ty, IsMem64, FlatTypes));
  }
  if (FlatTypes.size() > MaxFlat) {
    // Past the limit the values go to memory as a tuple: at the pointer the
    // caller gave, else at one allocated here and passed as the one slot.
    uint64_t TupleSize = 0;
    uint32_t TupleAlign = 1;
    for (const auto &Ty : Types) {
      EXPECTED_TRY(const uint32_t Align, getAlignment(TypeInst, Ty, IsMem64));
      EXPECTED_TRY(const uint64_t Size, getElemSize(TypeInst, Ty, IsMem64));
      TupleSize = alignTo(TupleSize, Align) + Size;
      TupleAlign = std::max(TupleAlign, Align);
    }
    TupleSize = alignTo(TupleSize, TupleAlign);
    uint64_t Ptr = 0;
    if (OutPtr.has_value()) {
      Ptr = *OutPtr;
      EXPECTED_TRY(checkAligned(Ptr, TupleAlign));
    } else {
      EXPECTED_TRY(Ptr, invokeRealloc(Opts, 0, 0, TupleAlign, TupleSize));
    }
    uint64_t Offset = 0;
    for (size_t I = 0; I < Types.size(); ++I) {
      EXPECTED_TRY(const uint32_t Align,
                   getAlignment(TypeInst, Types[I], IsMem64));
      EXPECTED_TRY(const uint64_t Size,
                   getElemSize(TypeInst, Types[I], IsMem64));
      Offset = alignTo(Offset, Align);
      EXPECTED_TRY(store(Opts, TypeInst, Types[I], Vals[I], Ptr + Offset));
      Offset += Size;
    }
    if (!OutPtr.has_value()) {
      Flat.push_back(Opts.newFlatPtr(Ptr));
    }
    return {};
  }
  for (size_t I = 0; I < Types.size(); ++I) {
    EXPECTED_TRY(lowerFlat(Opts, TypeInst, Types[I], Vals[I], Flat));
  }
  return {};
}

// Check a host value against its type. See
// "include/executor/component/executor.h".
Expect<void> ComponentExecutor::checkHostValue(
    const Runtime::Instance::ComponentInstance &TypeInst,
    const ComponentValType &Ty, const ComponentValVariant &Val) {
  if (Ty.isPrimValType()) {
    bool Ok = false;
    switch (Ty.getPrimValType()) {
    case PrimValType::Bool:
      Ok = std::holds_alternative<bool>(Val);
      break;
    case PrimValType::S8:
      Ok = std::holds_alternative<int8_t>(Val);
      break;
    case PrimValType::U8:
      Ok = std::holds_alternative<uint8_t>(Val);
      break;
    case PrimValType::S16:
      Ok = std::holds_alternative<int16_t>(Val);
      break;
    case PrimValType::U16:
      Ok = std::holds_alternative<uint16_t>(Val);
      break;
    case PrimValType::S32:
      Ok = std::holds_alternative<int32_t>(Val);
      break;
    case PrimValType::U32:
      Ok = std::holds_alternative<uint32_t>(Val);
      break;
    case PrimValType::S64:
      Ok = std::holds_alternative<int64_t>(Val);
      break;
    case PrimValType::U64:
      Ok = std::holds_alternative<uint64_t>(Val);
      break;
    case PrimValType::F32:
      Ok = std::holds_alternative<float>(Val);
      break;
    case PrimValType::F64:
      Ok = std::holds_alternative<double>(Val);
      break;
    case PrimValType::Char:
      Ok = std::holds_alternative<uint32_t>(Val) &&
           isValidChar(std::get<uint32_t>(Val));
      break;
    case PrimValType::String:
      Ok = std::holds_alternative<std::string>(Val);
      break;
    case PrimValType::ErrorContext:
      Ok = isComponentVal<ErrorContextVal>(Val);
      break;
    default:
      break;
    }
    return Ok ? Expect<void>{}
              : logMatchError(ComponentTypeCodeStr[Ty.getCode()]);
  }
  EXPECTED_TRY(auto Resolved, getDefValType(TypeInst, Ty));
  const auto &Def = *Resolved.first;
  const auto &Owner = *Resolved.second;
  if (Def.isPrimValType()) {
    return checkHostValue(Owner, ComponentValType(Def.getPrimValType()), Val);
  }
  if (Def.isRecordTy()) {
    if (!isComponentVal<RecordVal>(Val)) {
      return logMatchError("record"sv);
    }
    const auto &Record = getComponentVal<RecordVal>(Val);
    const auto &Types = Def.getRecord().LabelTypes;
    if (Record.Fields.size() != Types.size()) {
      return logMatchError("record field count"sv);
    }
    for (size_t I = 0; I < Types.size(); ++I) {
      EXPECTED_TRY(checkHostValue(Owner, Types[I].getValType(),
                                  Record.Fields[I].second));
    }
    return {};
  }
  if (Def.isTupleTy()) {
    if (!isComponentVal<TupleVal>(Val)) {
      return logMatchError("tuple"sv);
    }
    const auto &Tuple = getComponentVal<TupleVal>(Val);
    const auto &Types = Def.getTuple().Types;
    if (Tuple.Values.size() != Types.size()) {
      return logMatchError("tuple size"sv);
    }
    for (size_t I = 0; I < Types.size(); ++I) {
      EXPECTED_TRY(checkHostValue(Owner, Types[I], Tuple.Values[I]));
    }
    return {};
  }
  if (Def.isListTy() || Def.isMapTy()) {
    if (!isComponentVal<ListVal>(Val)) {
      return logMatchError("list"sv);
    }
    const auto &List = getComponentVal<ListVal>(Val);
    if (Def.isListTy()) {
      const auto &ListTy = Def.getList();
      if (ListTy.Len.has_value() && List.Elements.size() != *ListTy.Len) {
        return logMatchError("list length"sv);
      }
      for (const auto &Elem : List.Elements) {
        EXPECTED_TRY(checkHostValue(Owner, ListTy.ValTy, Elem));
      }
      return {};
    }
    for (const auto &Elem : List.Elements) {
      if (!isComponentVal<TupleVal>(Elem) ||
          getComponentVal<TupleVal>(Elem).Values.size() != 2) {
        return logMatchError("map entry"sv);
      }
      const auto &Entry = getComponentVal<TupleVal>(Elem);
      EXPECTED_TRY(checkHostValue(Owner, Def.getMap().KeyTy, Entry.Values[0]));
      EXPECTED_TRY(checkHostValue(Owner, Def.getMap().ValTy, Entry.Values[1]));
    }
    return {};
  }
  if (Def.isFlagsTy()) {
    if (!isComponentVal<FlagsVal>(Val)) {
      return logMatchError("flags"sv);
    }
    const auto &Flags = getComponentVal<FlagsVal>(Val);
    const auto &Labels = Def.getFlags().Labels;
    for (const auto &Set : Flags.SetLabels) {
      if (std::find(Labels.begin(), Labels.end(), Set) == Labels.end()) {
        return logMatchError("flags label"sv);
      }
    }
    if (Flags.SetLabels.empty() && Flags.Bits.size() > Labels.size()) {
      return logMatchError("flags size"sv);
    }
    return {};
  }
  if (Def.isOwnTy()) {
    return isComponentVal<OwnVal>(Val) ? Expect<void>{}
                                       : logMatchError("own"sv);
  }
  if (Def.isBorrowTy()) {
    return isComponentVal<BorrowVal>(Val) ? Expect<void>{}
                                          : logMatchError("borrow"sv);
  }
  if (Def.isStreamTy() || Def.isFutureTy()) {
    if (!isComponentVal<StreamFutureVal>(Val) ||
        getComponentVal<StreamFutureVal>(Val).IsStream != Def.isStreamTy()) {
      return logMatchError(Def.isStreamTy() ? "stream"sv : "future"sv);
    }
    return {};
  }
  if (Def.isOptionTy()) {
    if (!isComponentVal<OptionVal>(Val)) {
      return logMatchError("option"sv);
    }
    const auto &Option = getComponentVal<OptionVal>(Val);
    if (Option.Value.has_value()) {
      return checkHostValue(Owner, Def.getOption().ValTy, *Option.Value);
    }
    return {};
  }
  if (Def.isResultTy()) {
    if (!isComponentVal<ResultVal>(Val)) {
      return logMatchError("result"sv);
    }
    const auto &Res = getComponentVal<ResultVal>(Val);
    const auto &CaseTy =
        Res.IsOk ? Def.getResult().ValTy : Def.getResult().ErrTy;
    if (CaseTy.has_value() != Res.Payload.has_value()) {
      return logMatchError("result payload"sv);
    }
    if (CaseTy.has_value()) {
      return checkHostValue(Owner, *CaseTy, *Res.Payload);
    }
    return {};
  }
  if (Def.isEnumTy()) {
    if (!isComponentVal<EnumVal>(Val)) {
      return logMatchError("enum"sv);
    }
    const auto &Enum = getComponentVal<EnumVal>(Val);
    EXPECTED_TRY(findCaseIdx(Enum.Case, Enum.Label, Def.getEnum().Labels));
    return {};
  }
  if (Def.isVariantTy()) {
    if (!isComponentVal<VariantVal>(Val)) {
      return logMatchError("variant"sv);
    }
    const auto &Variant = getComponentVal<VariantVal>(Val);
    EXPECTED_TRY(const uint32_t Case, findCaseIdx(Variant.Case, Variant.Label,
                                                  getVariantLabels(Def)));
    const auto &Payload = Def.getVariant().Cases[Case].second;
    if (Payload.has_value() != Variant.Payload.has_value()) {
      return logMatchError("variant payload"sv);
    }
    if (Payload.has_value()) {
      return checkHostValue(Owner, *Payload, *Variant.Payload);
    }
    return {};
  }
  return logMatchError("type"sv);
}

// Structural type equality. See "include/executor/component/executor.h".
Expect<bool> ComponentExecutor::matchValType(
    const Runtime::Instance::ComponentInstance &AInst,
    const ComponentValType &A,
    const Runtime::Instance::ComponentInstance &BInst,
    const ComponentValType &B) {
  if (A.isPrimValType() || B.isPrimValType()) {
    if (A.isPrimValType() && B.isPrimValType()) {
      return A.getPrimValType() == B.getPrimValType();
    }
    // A named primitive equals the primitive it names.
    const auto &[Inst, Ty] =
        A.isPrimValType() ? std::tie(BInst, B) : std::tie(AInst, A);
    const auto &Prim = A.isPrimValType() ? A : B;
    EXPECTED_TRY(auto Resolved, getDefValType(Inst, Ty));
    return Resolved.first->isPrimValType() &&
           Resolved.first->getPrimValType() == Prim.getPrimValType();
  }
  EXPECTED_TRY(auto RA, getDefValType(AInst, A));
  EXPECTED_TRY(auto RB, getDefValType(BInst, B));
  const auto &DA = *RA.first;
  const auto &DB = *RB.first;
  const auto &OA = *RA.second;
  const auto &OB = *RB.second;
  if (DA.isPrimValType() || DB.isPrimValType()) {
    return DA.isPrimValType() && DB.isPrimValType() &&
           DA.getPrimValType() == DB.getPrimValType();
  }
  auto MatchOpt =
      [&](const std::optional<ComponentValType> &OptA,
          const std::optional<ComponentValType> &OptB) -> Expect<bool> {
    if (OptA.has_value() != OptB.has_value()) {
      return false;
    }
    if (!OptA.has_value()) {
      return true;
    }
    return matchValType(OA, *OptA, OB, *OptB);
  };
  if (DA.isRecordTy() && DB.isRecordTy()) {
    const auto &FA = DA.getRecord().LabelTypes;
    const auto &FB = DB.getRecord().LabelTypes;
    if (FA.size() != FB.size()) {
      return false;
    }
    for (size_t I = 0; I < FA.size(); ++I) {
      if (FA[I].getLabel() != FB[I].getLabel()) {
        return false;
      }
      EXPECTED_TRY(const bool Same, matchValType(OA, FA[I].getValType(), OB,
                                                 FB[I].getValType()));
      if (!Same) {
        return false;
      }
    }
    return true;
  }
  if (DA.isTupleTy() && DB.isTupleTy()) {
    const auto &TA = DA.getTuple().Types;
    const auto &TB = DB.getTuple().Types;
    if (TA.size() != TB.size()) {
      return false;
    }
    for (size_t I = 0; I < TA.size(); ++I) {
      EXPECTED_TRY(const bool Same, matchValType(OA, TA[I], OB, TB[I]));
      if (!Same) {
        return false;
      }
    }
    return true;
  }
  if (DA.isListTy() && DB.isListTy()) {
    if (DA.getList().Len != DB.getList().Len) {
      return false;
    }
    return matchValType(OA, DA.getList().ValTy, OB, DB.getList().ValTy);
  }
  if (DA.isMapTy() && DB.isMapTy()) {
    EXPECTED_TRY(const bool Keys,
                 matchValType(OA, DA.getMap().KeyTy, OB, DB.getMap().KeyTy));
    if (!Keys) {
      return false;
    }
    return matchValType(OA, DA.getMap().ValTy, OB, DB.getMap().ValTy);
  }
  if (DA.isFlagsTy() && DB.isFlagsTy()) {
    return DA.getFlags().Labels == DB.getFlags().Labels;
  }
  if (DA.isEnumTy() && DB.isEnumTy()) {
    return DA.getEnum().Labels == DB.getEnum().Labels;
  }
  if (DA.isOptionTy() && DB.isOptionTy()) {
    return matchValType(OA, DA.getOption().ValTy, OB, DB.getOption().ValTy);
  }
  if (DA.isResultTy() && DB.isResultTy()) {
    EXPECTED_TRY(const bool Ok,
                 MatchOpt(DA.getResult().ValTy, DB.getResult().ValTy));
    if (!Ok) {
      return false;
    }
    return MatchOpt(DA.getResult().ErrTy, DB.getResult().ErrTy);
  }
  if (DA.isVariantTy() && DB.isVariantTy()) {
    const auto &CA = DA.getVariant().Cases;
    const auto &CB = DB.getVariant().Cases;
    if (CA.size() != CB.size()) {
      return false;
    }
    for (size_t I = 0; I < CA.size(); ++I) {
      if (CA[I].first != CB[I].first) {
        return false;
      }
      EXPECTED_TRY(const bool Same, MatchOpt(CA[I].second, CB[I].second));
      if (!Same) {
        return false;
      }
    }
    return true;
  }
  if ((DA.isOwnTy() && DB.isOwnTy()) || (DA.isBorrowTy() && DB.isBorrowTy())) {
    // Resource types are nominal: the same runtime identity.
    const uint32_t IA = DA.isOwnTy() ? DA.getOwn().Idx : DA.getBorrow().Idx;
    const uint32_t IB = DB.isOwnTy() ? DB.getOwn().Idx : DB.getBorrow().Idx;
    EXPECTED_TRY(const auto *RTA, OA.getTypeResource(IA));
    EXPECTED_TRY(const auto *RTB, OB.getTypeResource(IB));
    return RTA != nullptr && RTA == RTB;
  }
  if ((DA.isStreamTy() && DB.isStreamTy()) ||
      (DA.isFutureTy() && DB.isFutureTy())) {
    const auto &EA =
        DA.isStreamTy() ? DA.getStream().ValTy : DA.getFuture().ValTy;
    const auto &EB =
        DB.isStreamTy() ? DB.getStream().ValTy : DB.getFuture().ValTy;
    return MatchOpt(EA, EB);
  }
  return false;
}

} // namespace Executor
} // namespace WasmEdge
