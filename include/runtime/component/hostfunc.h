// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/component/hostfunc.h - Host function interface ---===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the interface of the host component function class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/type.h"
#include "common/component_variant.h"
#include "common/errcode.h"
#include "common/span.h"
#include "runtime/component/wit.h"

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Component {

class CallingFrame;

/// A host-implemented component function over component-level values.
class HostFunctionBase {
public:
  HostFunctionBase() = default;
  virtual ~HostFunctionBase() = default;

  /// Run host function body.
  virtual Expect<void> run(CallingFrame &Frame,
                           Span<const ComponentValVariant> Args,
                           Span<ComponentValVariant> Rets) = 0;

  /// Declare the function type through the minter.
  virtual void declare(const TypeMinter &) noexcept {}

  /// Getter for function type.
  const AST::Component::FuncType &getFuncType() const noexcept {
    return DeclType;
  }

protected:
  AST::Component::FuncType DeclType;
};

/// A host function over a typed body `Expect<R> T::body(CallingFrame &, A...)`.
/// T names the parameters in `static constexpr const char *ParamNames[]`.
template <typename T> class HostFunction : public HostFunctionBase {
public:
  explicit HostFunction(bool Async = false) noexcept : IsAsync(Async) {}

  Expect<void> run(CallingFrame &Frame, Span<const ComponentValVariant> Args,
                   Span<ComponentValVariant> Rets) override {
    using F = FuncTraits<decltype(&T::body)>;
    if (unlikely(F::ArgsN != Args.size())) {
      return Unexpect(ErrCode::Value::FuncSigMismatch);
    }
    if (unlikely(F::RetsN != Rets.size())) {
      return Unexpect(ErrCode::Value::FuncSigMismatch);
    }
    return invoke(Frame, Args.first<F::ArgsN>(), Rets.first<F::RetsN>());
  }

  void declare(const TypeMinter &Mint) noexcept override {
    using F = FuncTraits<decltype(&T::body)>;
    std::vector<AST::Component::LabelValType> Params;
    Params.reserve(F::ArgsN);
    if constexpr (F::ArgsN > 0) {
      static_assert(std::size(T::ParamNames) == F::ArgsN,
                    "ParamNames must name every parameter of body");
      pushParamTypes<typename F::ArgsT>(Mint, Params,
                                        std::make_index_sequence<F::ArgsN>());
    }
    std::optional<ComponentValType> Result;
    if constexpr (F::HasResult) {
      Result = Wit<typename F::RetT>::type(Mint);
    }
    DeclType = AST::Component::FuncType(std::move(Params), Result);
    DeclType.setAsync(IsAsync);
  }

private:
  template <typename> struct FuncTraits;
  template <typename R, typename C, typename... A>
  struct FuncTraits<Expect<R> (C::*)(CallingFrame &, A...)> {
    using ArgsT = std::tuple<A...>;
    using RetT = R;
    static inline constexpr const std::size_t ArgsN = sizeof...(A);
    static inline constexpr const std::size_t RetsN = 1;
    static inline constexpr const bool HasResult = true;
  };
  template <typename C, typename... A>
  struct FuncTraits<Expect<void> (C::*)(CallingFrame &, A...)> {
    using ArgsT = std::tuple<A...>;
    using RetT = void;
    static inline constexpr const std::size_t ArgsN = sizeof...(A);
    static inline constexpr const std::size_t RetsN = 0;
    static inline constexpr const bool HasResult = false;
  };

  template <typename SpanA, typename SpanR>
  Expect<void> invoke(CallingFrame &Frame, SpanA &&Args, SpanR &&Rets) {
    using F = FuncTraits<decltype(&T::body)>;
    using ArgsT = typename F::ArgsT;

    auto GeneralArguments = std::tie(*static_cast<T *>(this), Frame);
    auto ArgTuple = toTuple<ArgsT>(std::forward<SpanA>(Args),
                                   std::make_index_sequence<F::ArgsN>());
    auto FuncArgTuple =
        std::tuple_cat(std::move(GeneralArguments), std::move(ArgTuple));
    if constexpr (F::HasResult) {
      EXPECTED_TRY(auto Ret, std::apply(&T::body, std::move(FuncArgTuple)));
      std::forward<SpanR>(Rets)[0] =
          Wit<typename F::RetT>::into(std::move(Ret));
    } else {
      EXPECTED_TRY(std::apply(&T::body, std::move(FuncArgTuple)));
    }
    return {};
  }

  template <typename Tuple, typename SpanT, std::size_t... Indices>
  static Tuple toTuple(SpanT &&Args, std::index_sequence<Indices...>) {
    return Tuple(Wit<std::tuple_element_t<Indices, Tuple>>::from(
        std::forward<SpanT>(Args)[Indices])...);
  }

  template <typename Tuple, std::size_t... Indices>
  static void pushParamTypes(const TypeMinter &Mint,
                             std::vector<AST::Component::LabelValType> &Params,
                             std::index_sequence<Indices...>) noexcept {
    (Params.emplace_back(std::string(T::ParamNames[Indices]),
                         Wit<std::tuple_element_t<Indices, Tuple>>::type(Mint)),
     ...);
  }

  bool IsAsync;
};

/// The table of the host objects behind a resource type; a representation
/// packs the generation of a slot over its one-based number.
template <typename T, typename Holder = std::unique_ptr<T>>
class ResourceTable {
public:
  uint64_t add(Holder Obj) noexcept {
    uint64_t Slot = Slots.size();
    if (!Free.empty()) {
      Slot = Free.back();
      Free.pop_back();
      Slots[Slot] = std::move(Obj);
    } else {
      Slots.push_back(std::move(Obj));
      Generations.push_back(0);
    }
    return (static_cast<uint64_t>(Generations[Slot]) << 32) | (Slot + 1);
  }

  /// The object behind a representation, null when there is none.
  T *get(uint64_t Rep) const noexcept {
    const Holder *Obj = getHolder(Rep);
    return Obj == nullptr ? nullptr : Obj->get();
  }

  /// The holder of the object behind a representation, null when none.
  const Holder *getHolder(uint64_t Rep) const noexcept {
    const uint64_t Slot = (Rep & UINT64_C(0xFFFFFFFF)) - 1;
    if ((Rep & UINT64_C(0xFFFFFFFF)) == 0 || Slot >= Slots.size() ||
        Generations[Slot] != static_cast<uint32_t>(Rep >> 32) || !Slots[Slot]) {
      return nullptr;
    }
    return &Slots[Slot];
  }

  Holder remove(uint64_t Rep) noexcept {
    if (getHolder(Rep) == nullptr) {
      return nullptr;
    }
    const uint64_t Slot = (Rep & UINT64_C(0xFFFFFFFF)) - 1;
    Free.push_back(Slot);
    Generations[Slot] += 1;
    return std::move(Slots[Slot]);
  }

  /// The number of live objects.
  std::size_t size() const noexcept { return Slots.size() - Free.size(); }

private:
  std::vector<Holder> Slots;
  std::vector<uint32_t> Generations;
  std::vector<uint64_t> Free;
};

} // namespace Component
} // namespace Runtime
} // namespace WasmEdge
