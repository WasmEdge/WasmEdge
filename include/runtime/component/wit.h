// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/component/wit.h - WIT type mapping definition ----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the mapping between C++ types and component value types.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/type.h"
#include "common/component_valtype.h"
#include "common/component_variant.h"
#include "common/expected.h"
#include "common/span.h"

#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Component {

/// Mints type indices in the type index space of a host instance.
class TypeMinter {
public:
  using DefTypeCallback = std::function<uint32_t(AST::Component::DefType &&)>;
  using ResourceCallback = std::function<uint32_t(std::type_index)>;

  TypeMinter(DefTypeCallback DefCallback,
             ResourceCallback ResCallback = {}) noexcept
      : Def(std::move(DefCallback)), Res(std::move(ResCallback)) {}

  /// Wrap a defined value type and mint the type index the instance owns.
  ComponentValType defValType(AST::Component::DefValType &&DefValTy) const {
    AST::Component::DefType DefTy;
    DefTy.setDefValType(std::move(DefValTy));
    return ComponentValType(Def(std::move(DefTy)));
  }
  /// Mint a named primitive type, as a WIT `type name = prim` declares.
  ComponentValType primDefType(PrimValType Prim) const {
    AST::Component::DefValType DefValTy;
    DefValTy.setPrimValType(Prim);
    return defValType(std::move(DefValTy));
  }

  /// The index of the resource type registered under the tag T.
  template <typename T> uint32_t getResourceIndex() const {
    return Res(std::type_index(typeid(T)));
  }

private:
  DefTypeCallback Def;
  ResourceCallback Res;
};

/// The component type of a C++ type and the conversions of its values.
template <typename T> struct Wit;

/// The aggregate held by a component value.
inline const ValComp &valComp(const ComponentValVariant &Val) {
  return *std::get<std::shared_ptr<ValComp>>(Val);
}

/// A primitive type: the C++ type is the variant arm itself.
template <typename T, ComponentTypeCode Code> struct WitPrim {
  static ComponentValType type(const TypeMinter &) noexcept {
    return ComponentValType(Code);
  }
  static T from(const ComponentValVariant &Val) { return std::get<T>(Val); }
  static ComponentValVariant into(T Val) noexcept {
    return ComponentValVariant{std::move(Val)};
  }
};

template <> struct Wit<bool> : WitPrim<bool, ComponentTypeCode::Bool> {};
template <> struct Wit<int8_t> : WitPrim<int8_t, ComponentTypeCode::S8> {};
template <> struct Wit<uint8_t> : WitPrim<uint8_t, ComponentTypeCode::U8> {};
template <> struct Wit<int16_t> : WitPrim<int16_t, ComponentTypeCode::S16> {};
template <> struct Wit<uint16_t> : WitPrim<uint16_t, ComponentTypeCode::U16> {};
template <> struct Wit<int32_t> : WitPrim<int32_t, ComponentTypeCode::S32> {};
template <> struct Wit<uint32_t> : WitPrim<uint32_t, ComponentTypeCode::U32> {};
template <> struct Wit<int64_t> : WitPrim<int64_t, ComponentTypeCode::S64> {};
template <> struct Wit<uint64_t> : WitPrim<uint64_t, ComponentTypeCode::U64> {};
template <> struct Wit<float> : WitPrim<float, ComponentTypeCode::F32> {};
template <> struct Wit<double> : WitPrim<double, ComponentTypeCode::F64> {};
template <>
struct Wit<std::string> : WitPrim<std::string, ComponentTypeCode::String> {};

/// list<T>
template <typename T> struct Wit<std::vector<T>> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    AST::Component::DefValType DefValTy;
    DefValTy.setList(AST::Component::ListTy{Wit<T>::type(Mint), std::nullopt});
    return Mint.defValType(std::move(DefValTy));
  }
  static std::vector<T> from(const ComponentValVariant &Val) {
    const auto &List = std::get<ListVal>(valComp(Val).V);
    std::vector<T> Out;
    Out.reserve(List.Elements.size());
    for (const auto &Elem : List.Elements) {
      Out.push_back(Wit<T>::from(Elem));
    }
    return Out;
  }
  static ComponentValVariant into(std::vector<T> &&Vals) noexcept {
    ListVal List;
    List.Elements.reserve(Vals.size());
    for (auto &Elem : Vals) {
      List.Elements.push_back(Wit<T>::into(std::move(Elem)));
    }
    return makeComponentVal(std::move(List));
  }
};

/// option<T>
template <typename T> struct Wit<std::optional<T>> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    AST::Component::DefValType DefValTy;
    DefValTy.setOption(AST::Component::OptionTy{Wit<T>::type(Mint)});
    return Mint.defValType(std::move(DefValTy));
  }
  static std::optional<T> from(const ComponentValVariant &Val) {
    const auto &Option = std::get<OptionVal>(valComp(Val).V);
    if (!Option.Value.has_value()) {
      return std::nullopt;
    }
    return Wit<T>::from(*Option.Value);
  }
  static ComponentValVariant into(std::optional<T> &&Val) noexcept {
    OptionVal Option;
    if (Val.has_value()) {
      Option.Value = Wit<T>::into(std::move(*Val));
    }
    return makeComponentVal(std::move(Option));
  }
};

/// tuple<T...>
template <typename... Ts> struct Wit<std::tuple<Ts...>> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    AST::Component::TupleTy Tuple;
    Tuple.Types = {Wit<Ts>::type(Mint)...};
    AST::Component::DefValType DefValTy;
    DefValTy.setTuple(std::move(Tuple));
    return Mint.defValType(std::move(DefValTy));
  }
  static std::tuple<Ts...> from(const ComponentValVariant &Val) {
    const auto &Tuple = std::get<TupleVal>(valComp(Val).V);
    return fromValues(Tuple.Values, std::index_sequence_for<Ts...>());
  }
  static ComponentValVariant into(std::tuple<Ts...> &&Val) noexcept {
    return intoValues(std::move(Val), std::index_sequence_for<Ts...>());
  }

private:
  template <std::size_t... I>
  static std::tuple<Ts...>
  fromValues(const std::vector<ComponentValVariant> &Vals,
             std::index_sequence<I...>) {
    return std::tuple<Ts...>(Wit<Ts>::from(Vals[I])...);
  }
  template <std::size_t... I>
  static ComponentValVariant intoValues(std::tuple<Ts...> &&Val,
                                        std::index_sequence<I...>) noexcept {
    TupleVal Tuple;
    Tuple.Values = {Wit<Ts>::into(std::get<I>(std::move(Val)))...};
    return makeComponentVal(std::move(Tuple));
  }
};

/// The absent payload of a WIT `result`: `result<_, E>` is `Expected<WitUnit,
/// E>` and `result<T>` is `Expected<T, WitUnit>`.
struct WitUnit {};

/// result<T, E>
template <typename T, typename E> struct Wit<Expected<T, E>> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    AST::Component::ResultTy Result;
    if constexpr (!std::is_same_v<T, WitUnit>) {
      Result.ValTy = Wit<T>::type(Mint);
    }
    if constexpr (!std::is_same_v<E, WitUnit>) {
      Result.ErrTy = Wit<E>::type(Mint);
    }
    AST::Component::DefValType DefValTy;
    DefValTy.setResult(std::move(Result));
    return Mint.defValType(std::move(DefValTy));
  }
  static Expected<T, E> from(const ComponentValVariant &Val) {
    const auto &Result = std::get<ResultVal>(valComp(Val).V);
    if (Result.IsOk) {
      if constexpr (std::is_same_v<T, WitUnit>) {
        return WitUnit{};
      } else {
        return Wit<T>::from(*Result.Payload);
      }
    }
    if constexpr (std::is_same_v<E, WitUnit>) {
      return Unexpected<E>(WitUnit{});
    } else {
      return Unexpected<E>(Wit<E>::from(*Result.Payload));
    }
  }
  static ComponentValVariant into(Expected<T, E> &&Val) noexcept {
    ResultVal Result;
    Result.IsOk = Val.has_value();
    if (Result.IsOk) {
      if constexpr (!std::is_same_v<T, WitUnit>) {
        Result.Payload = Wit<T>::into(std::move(*Val));
      }
    } else if constexpr (!std::is_same_v<E, WitUnit>) {
      Result.Payload = Wit<E>::into(std::move(Val.error()));
    }
    return makeComponentVal(std::move(Result));
  }
};

/// Helpers of a Wit<T> specialization over a C++ struct for a WIT record.
struct WitRecord {
  static ComponentValType
  type(const TypeMinter &Mint,
       std::vector<AST::Component::LabelValType> &&Fields) noexcept {
    AST::Component::RecordTy Record;
    Record.LabelTypes = std::move(Fields);
    AST::Component::DefValType DefValTy;
    DefValTy.setRecord(std::move(Record));
    return Mint.defValType(std::move(DefValTy));
  }
  static Span<const std::pair<std::string, ComponentValVariant>>
  fields(const ComponentValVariant &Val) {
    return std::get<RecordVal>(valComp(Val).V).Fields;
  }
  static ComponentValVariant
  into(std::vector<std::pair<std::string, ComponentValVariant>>
           &&Fields) noexcept {
    return makeComponentVal(RecordVal{std::move(Fields)});
  }
};

/// Helpers of a Wit<T> specialization over a C++ type for a WIT variant.
struct WitVariant {
  using Cases =
      std::vector<std::pair<std::string, std::optional<ComponentValType>>>;
  static ComponentValType type(const TypeMinter &Mint,
                               Cases &&CaseTypes) noexcept {
    AST::Component::VariantTy Ty;
    Ty.Cases = std::move(CaseTypes);
    AST::Component::DefValType DefValTy;
    DefValTy.setVariant(std::move(Ty));
    return Mint.defValType(std::move(DefValTy));
  }
  static const VariantVal &value(const ComponentValVariant &Val) {
    return std::get<VariantVal>(valComp(Val).V);
  }
  static ComponentValVariant
  into(uint32_t Case,
       std::optional<ComponentValVariant> Payload = std::nullopt) noexcept {
    return makeComponentVal(VariantVal{Case, std::move(Payload), {}});
  }
};

/// Wit<E> base for a C++ enum class E over a WIT enum. The specialization
/// lists the cases in order in `static constexpr const char *Labels[]`.
template <typename E, typename Derived> struct WitEnum {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    AST::Component::EnumTy Ty;
    for (const char *Label : Derived::Labels) {
      Ty.Labels.emplace_back(Label);
    }
    AST::Component::DefValType DefValTy;
    DefValTy.setEnum(std::move(Ty));
    return Mint.defValType(std::move(DefValTy));
  }
  static E from(const ComponentValVariant &Val) {
    return static_cast<E>(std::get<EnumVal>(valComp(Val).V).Case);
  }
  static ComponentValVariant into(E Val) noexcept {
    return makeComponentVal(EnumVal{static_cast<uint32_t>(Val), {}});
  }
};

/// Wit<F> base for a C++ enum class F of bit values over a WIT flags type.
/// The specialization lists the labels in bit order in `Labels[]`.
template <typename F, typename Derived> struct WitFlags {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    AST::Component::FlagsTy Ty;
    for (const char *Label : Derived::Labels) {
      Ty.Labels.emplace_back(Label);
    }
    AST::Component::DefValType DefValTy;
    DefValTy.setFlags(std::move(Ty));
    return Mint.defValType(std::move(DefValTy));
  }
  static F from(const ComponentValVariant &Val) {
    const auto &Bits = std::get<FlagsVal>(valComp(Val).V).Bits;
    uint32_t Packed = 0;
    for (size_t I = 0; I < Bits.size() && I < 32; ++I) {
      if (Bits[I]) {
        Packed |= (1U << I);
      }
    }
    return static_cast<F>(Packed);
  }
  static ComponentValVariant into(F Val) noexcept {
    FlagsVal Flags;
    const auto Packed = static_cast<uint32_t>(Val);
    Flags.Bits.resize(std::size(Derived::Labels));
    for (size_t I = 0; I < Flags.Bits.size(); ++I) {
      Flags.Bits[I] = ((Packed >> I) & 1U) != 0;
    }
    return makeComponentVal(std::move(Flags));
  }
};

/// `own<R>` and `borrow<R>` of the host resource type under the tag R.
template <typename R> struct Own {
  uint64_t Rep = 0;
};
template <typename R> struct Borrow {
  uint64_t Rep = 0;
};

template <typename R> struct Wit<Own<R>> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    AST::Component::DefValType DefValTy;
    DefValTy.setOwn(AST::Component::OwnTy{Mint.getResourceIndex<R>()});
    return Mint.defValType(std::move(DefValTy));
  }
  static Own<R> from(const ComponentValVariant &Val) {
    return Own<R>{std::get<OwnVal>(valComp(Val).V).Handle};
  }
  static ComponentValVariant into(Own<R> Val) noexcept {
    return makeComponentVal(OwnVal{Val.Rep});
  }
};

template <typename R> struct Wit<Borrow<R>> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    AST::Component::DefValType DefValTy;
    DefValTy.setBorrow(AST::Component::BorrowTy{Mint.getResourceIndex<R>()});
    return Mint.defValType(std::move(DefValTy));
  }
  static Borrow<R> from(const ComponentValVariant &Val) {
    return Borrow<R>{std::get<BorrowVal>(valComp(Val).V).Handle};
  }
  static ComponentValVariant into(Borrow<R> Val) noexcept {
    return makeComponentVal(BorrowVal{Val.Rep});
  }
};

/// `stream<T>` and `future<T>` as the handle of the readable end in the table
/// of the root of the host; T is void for no payload.
template <typename T> struct Stream {
  uint32_t HandleIdx = 0;
};
template <typename T> struct Future {
  uint32_t HandleIdx = 0;
};

template <typename T> struct Wit<Stream<T>> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    AST::Component::StreamTy Ty;
    if constexpr (!std::is_void_v<T>) {
      Ty.ValTy = Wit<T>::type(Mint);
    }
    AST::Component::DefValType DefValTy;
    DefValTy.setStream(std::move(Ty));
    return Mint.defValType(std::move(DefValTy));
  }
  static Stream<T> from(const ComponentValVariant &Val) {
    return Stream<T>{std::get<StreamFutureVal>(valComp(Val).V).HandleIdx};
  }
  static ComponentValVariant into(Stream<T> &&Val) noexcept {
    return makeComponentVal(StreamFutureVal{nullptr, Val.HandleIdx, true});
  }
};

template <typename T> struct Wit<Future<T>> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    AST::Component::FutureTy Ty;
    if constexpr (!std::is_void_v<T>) {
      Ty.ValTy = Wit<T>::type(Mint);
    }
    AST::Component::DefValType DefValTy;
    DefValTy.setFuture(std::move(Ty));
    return Mint.defValType(std::move(DefValTy));
  }
  static Future<T> from(const ComponentValVariant &Val) {
    return Future<T>{std::get<StreamFutureVal>(valComp(Val).V).HandleIdx};
  }
  static ComponentValVariant into(Future<T> &&Val) noexcept {
    return makeComponentVal(StreamFutureVal{nullptr, Val.HandleIdx, false});
  }
};

} // namespace Component
} // namespace Runtime
} // namespace WasmEdge
