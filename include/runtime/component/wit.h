// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/component/wit.h - WIT type mapping definition ----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the mapping between C++ types and component model value
/// types, which host component functions declare their types and convert
/// their values through.
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

/// Mints type indices in the type space of a host instance: defined types by
/// value, and the resource types the instance registered under a C++ tag.
class TypeMinter {
public:
  using DefTypeCallback = std::function<uint32_t(AST::Component::DefType &&)>;
  using ResourceCallback = std::function<uint32_t(std::type_index)>;

  TypeMinter(DefTypeCallback D, ResourceCallback R = {}) noexcept
      : Def(std::move(D)), Res(std::move(R)) {}

  /// Mint the index of a defined type the instance takes ownership of.
  uint32_t defType(AST::Component::DefType &&Ty) const {
    return Def(std::move(Ty));
  }
  /// Wrap a defined value type and mint its type index.
  ComponentValType defValType(AST::Component::DefValType &&DVT) const {
    AST::Component::DefType DT;
    DT.setDefValType(std::move(DVT));
    return ComponentValType(defType(std::move(DT)));
  }
  /// Mint a named primitive type, as a WIT `type name = prim` declares.
  ComponentValType primDefType(PrimValType Prim) const {
    AST::Component::DefValType DVT;
    DVT.setPrimValType(Prim);
    return defValType(std::move(DVT));
  }

  /// The index of the resource type registered under Tag.
  uint32_t getResourceIndex(std::type_index Tag) const { return Res(Tag); }
  template <typename T> uint32_t getResourceIndex() const {
    return Res(std::type_index(typeid(T)));
  }

private:
  DefTypeCallback Def;
  ResourceCallback Res;
};

/// The component model type of a C++ type, with the conversions between the
/// C++ value and its ComponentValVariant form. An unsupported C++ type has no
/// definition, so a host function over it fails to compile.
template <typename T> struct Wit;

/// The aggregate held by a component value.
inline const ValComp &valComp(const ComponentValVariant &V) {
  return *std::get<std::shared_ptr<ValComp>>(V);
}

/// A primitive type: the C++ type is the variant arm itself.
template <typename T, ComponentTypeCode Code> struct WitPrim {
  static ComponentValType type(const TypeMinter &) noexcept {
    return ComponentValType(Code);
  }
  static T from(const ComponentValVariant &V) { return std::get<T>(V); }
  static ComponentValVariant into(T V) noexcept {
    return ComponentValVariant{std::move(V)};
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
    AST::Component::DefValType DVT;
    DVT.setList(AST::Component::ListTy{Wit<T>::type(Mint), std::nullopt});
    return Mint.defValType(std::move(DVT));
  }
  static std::vector<T> from(const ComponentValVariant &V) {
    const auto &L = std::get<ListVal>(valComp(V).V);
    std::vector<T> Out;
    Out.reserve(L.Elements.size());
    for (const auto &E : L.Elements) {
      Out.push_back(Wit<T>::from(E));
    }
    return Out;
  }
  static ComponentValVariant into(std::vector<T> &&Vals) noexcept {
    ListVal L;
    L.Elements.reserve(Vals.size());
    for (auto &E : Vals) {
      L.Elements.push_back(Wit<T>::into(std::move(E)));
    }
    return makeComponentVal(std::move(L));
  }
};

/// option<T>
template <typename T> struct Wit<std::optional<T>> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    AST::Component::DefValType DVT;
    DVT.setOption(AST::Component::OptionTy{Wit<T>::type(Mint)});
    return Mint.defValType(std::move(DVT));
  }
  static std::optional<T> from(const ComponentValVariant &V) {
    const auto &O = std::get<OptionVal>(valComp(V).V);
    if (!O.Value.has_value()) {
      return std::nullopt;
    }
    return Wit<T>::from(*O.Value);
  }
  static ComponentValVariant into(std::optional<T> &&Val) noexcept {
    OptionVal O;
    if (Val.has_value()) {
      O.Value = Wit<T>::into(std::move(*Val));
    }
    return makeComponentVal(std::move(O));
  }
};

/// tuple<T...>
template <typename... Ts> struct Wit<std::tuple<Ts...>> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    AST::Component::TupleTy Tu;
    Tu.Types = {Wit<Ts>::type(Mint)...};
    AST::Component::DefValType DVT;
    DVT.setTuple(std::move(Tu));
    return Mint.defValType(std::move(DVT));
  }
  static std::tuple<Ts...> from(const ComponentValVariant &V) {
    const auto &Tu = std::get<TupleVal>(valComp(V).V);
    return fromValues(Tu.Values, std::index_sequence_for<Ts...>());
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
    TupleVal Tu;
    Tu.Values = {Wit<Ts>::into(std::get<I>(std::move(Val)))...};
    return makeComponentVal(std::move(Tu));
  }
};

/// The absent payload of a WIT `result`: `result<_, E>` is `Expected<Unit, E>`
/// and `result<T>` is `Expected<T, Unit>`.
struct Unit {};

/// result<T, E>
template <typename T, typename E> struct Wit<Expected<T, E>> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    AST::Component::ResultTy R;
    if constexpr (!std::is_same_v<T, Unit>) {
      R.ValTy = Wit<T>::type(Mint);
    }
    if constexpr (!std::is_same_v<E, Unit>) {
      R.ErrTy = Wit<E>::type(Mint);
    }
    AST::Component::DefValType DVT;
    DVT.setResult(std::move(R));
    return Mint.defValType(std::move(DVT));
  }
  static Expected<T, E> from(const ComponentValVariant &V) {
    const auto &R = std::get<ResultVal>(valComp(V).V);
    if (R.IsOk) {
      if constexpr (std::is_same_v<T, Unit>) {
        return Unit{};
      } else {
        return Wit<T>::from(*R.Payload);
      }
    }
    if constexpr (std::is_same_v<E, Unit>) {
      return Unexpected<E>(Unit{});
    } else {
      return Unexpected<E>(Wit<E>::from(*R.Payload));
    }
  }
  static ComponentValVariant into(Expected<T, E> &&Val) noexcept {
    ResultVal R;
    R.IsOk = Val.has_value();
    if (R.IsOk) {
      if constexpr (!std::is_same_v<T, Unit>) {
        R.Payload = Wit<T>::into(std::move(*Val));
      }
    } else if constexpr (!std::is_same_v<E, Unit>) {
      R.Payload = Wit<E>::into(std::move(Val.error()));
    }
    return makeComponentVal(std::move(R));
  }
};

/// The pieces of a Wit<T> specialization over a C++ struct standing for a
/// WIT record: the type from its labelled fields, and the value both ways.
struct WitRecord {
  static ComponentValType
  type(const TypeMinter &Mint,
       std::vector<AST::Component::LabelValType> &&Fields) noexcept {
    AST::Component::RecordTy R;
    R.LabelTypes = std::move(Fields);
    AST::Component::DefValType DVT;
    DVT.setRecord(std::move(R));
    return Mint.defValType(std::move(DVT));
  }
  static Span<const std::pair<std::string, ComponentValVariant>>
  fields(const ComponentValVariant &V) {
    return std::get<RecordVal>(valComp(V).V).Fields;
  }
  static ComponentValVariant
  into(std::vector<std::pair<std::string, ComponentValVariant>>
           &&Fields) noexcept {
    return makeComponentVal(RecordVal{std::move(Fields)});
  }
};

/// The pieces of a Wit<T> specialization over a C++ type standing for a WIT
/// variant: the type from its labelled cases, and the value both ways.
struct WitVariant {
  using Cases =
      std::vector<std::pair<std::string, std::optional<ComponentValType>>>;
  static ComponentValType type(const TypeMinter &Mint, Cases &&Cs) noexcept {
    AST::Component::VariantTy Ty;
    Ty.Cases = std::move(Cs);
    AST::Component::DefValType DVT;
    DVT.setVariant(std::move(Ty));
    return Mint.defValType(std::move(DVT));
  }
  static const VariantVal &value(const ComponentValVariant &V) {
    return std::get<VariantVal>(valComp(V).V);
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
    for (const char *L : Derived::Labels) {
      Ty.Labels.emplace_back(L);
    }
    AST::Component::DefValType DVT;
    DVT.setEnum(std::move(Ty));
    return Mint.defValType(std::move(DVT));
  }
  static E from(const ComponentValVariant &V) {
    return static_cast<E>(std::get<EnumVal>(valComp(V).V).Case);
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
    for (const char *L : Derived::Labels) {
      Ty.Labels.emplace_back(L);
    }
    AST::Component::DefValType DVT;
    DVT.setFlags(std::move(Ty));
    return Mint.defValType(std::move(DVT));
  }
  static F from(const ComponentValVariant &V) {
    const auto &Bits = std::get<FlagsVal>(valComp(V).V).Bits;
    uint32_t Packed = 0;
    for (size_t I = 0; I < Bits.size() && I < 32; ++I) {
      if (Bits[I]) {
        Packed |= (1U << I);
      }
    }
    return static_cast<F>(Packed);
  }
  static ComponentValVariant into(F Val) noexcept {
    FlagsVal FV;
    const auto Packed = static_cast<uint32_t>(Val);
    FV.Bits.resize(std::size(Derived::Labels));
    for (size_t I = 0; I < FV.Bits.size(); ++I) {
      FV.Bits[I] = ((Packed >> I) & 1U) != 0;
    }
    return makeComponentVal(std::move(FV));
  }
};

/// `own<R>` and `borrow<R>` of the host resource type registered under the
/// tag R. The value carries the host representation of the resource.
template <typename R> struct Own {
  uint64_t Rep = 0;
};
template <typename R> struct Borrow {
  uint64_t Rep = 0;
};

template <typename R> struct Wit<Own<R>> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    AST::Component::DefValType DVT;
    DVT.setOwn(AST::Component::OwnTy{Mint.getResourceIndex<R>()});
    return Mint.defValType(std::move(DVT));
  }
  static Own<R> from(const ComponentValVariant &V) {
    return Own<R>{std::get<OwnVal>(valComp(V).V).Handle};
  }
  static ComponentValVariant into(Own<R> Val) noexcept {
    return makeComponentVal(OwnVal{Val.Rep});
  }
};

template <typename R> struct Wit<Borrow<R>> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    AST::Component::DefValType DVT;
    DVT.setBorrow(AST::Component::BorrowTy{Mint.getResourceIndex<R>()});
    return Mint.defValType(std::move(DVT));
  }
  static Borrow<R> from(const ComponentValVariant &V) {
    return Borrow<R>{std::get<BorrowVal>(valComp(V).V).Handle};
  }
  static ComponentValVariant into(Borrow<R> Val) noexcept {
    return makeComponentVal(BorrowVal{Val.Rep});
  }
};

/// `stream<T>` and `future<T>`: the transferred end as the type-erased
/// rendezvous state it shares with its peer. T is void for no payload.
template <typename T> struct Stream {
  std::shared_ptr<void> Shared;
};
template <typename T> struct Future {
  std::shared_ptr<void> Shared;
};

template <typename T> struct Wit<Stream<T>> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    AST::Component::StreamTy Ty;
    if constexpr (!std::is_void_v<T>) {
      Ty.ValTy = Wit<T>::type(Mint);
    }
    AST::Component::DefValType DVT;
    DVT.setStream(std::move(Ty));
    return Mint.defValType(std::move(DVT));
  }
  static Stream<T> from(const ComponentValVariant &V) {
    return Stream<T>{std::get<StreamFutureVal>(valComp(V).V).Shared};
  }
  static ComponentValVariant into(Stream<T> &&Val) noexcept {
    return makeComponentVal(StreamFutureVal{std::move(Val.Shared), true});
  }
};

template <typename T> struct Wit<Future<T>> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    AST::Component::FutureTy Ty;
    if constexpr (!std::is_void_v<T>) {
      Ty.ValTy = Wit<T>::type(Mint);
    }
    AST::Component::DefValType DVT;
    DVT.setFuture(std::move(Ty));
    return Mint.defValType(std::move(DVT));
  }
  static Future<T> from(const ComponentValVariant &V) {
    return Future<T>{std::get<StreamFutureVal>(valComp(V).V).Shared};
  }
  static ComponentValVariant into(Future<T> &&Val) noexcept {
    return makeComponentVal(StreamFutureVal{std::move(Val.Shared), false});
  }
};

} // namespace Component
} // namespace Runtime
} // namespace WasmEdge
