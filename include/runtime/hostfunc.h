// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/runtime/hostfunc.h - host function interface -------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the interface of host function class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/type.h"
#include "common/errcode.h"
#include "common/span.h"
#include "common/types.h"

#include <memory>
#include <tuple>
#include <vector>

namespace WasmEdge {
namespace Runtime {

class CallingFrame;

class HostFunctionBase {
public:
  HostFunctionBase() = delete;
  HostFunctionBase(const uint64_t FuncCost)
      : DefType(AST::FunctionType()), Cost(FuncCost) {}
  virtual ~HostFunctionBase() = default;

  /// Run host function body.
  virtual Expect<void> run(const CallingFrame &CallFrame,
                           Span<const ValVariant> Args,
                           Span<ValVariant> Rets) = 0;

  /// Getter of function type.
  const AST::FunctionType &getFuncType() const noexcept {
    return DefType.getCompositeType().getFuncType();
  }

  /// Getter of host function cost.
  uint64_t getCost() const { return Cost; }

  /// Getter of defined type.
  const AST::SubType &getDefinedType() const noexcept { return DefType; }

protected:
  AST::SubType DefType;
  const uint64_t Cost;
};

template <typename T> class HostFunction : public HostFunctionBase {
public:
  HostFunction(const uint64_t FuncCost = 0) : HostFunctionBase(FuncCost) {
    initializeFuncType();
  }

  Expect<void> run(const CallingFrame &CallFrame, Span<const ValVariant> Args,
                   Span<ValVariant> Rets) override {
    using F = FuncTraits<decltype(&T::body)>;
    if (unlikely(F::ArgsN != Args.size())) {
      return Unexpect(ErrCode::Value::FuncSigMismatch);
    }
    if (unlikely(F::RetsN != Rets.size())) {
      return Unexpect(ErrCode::Value::FuncSigMismatch);
    }
    return invoke(CallFrame, Args.first<F::ArgsN>(), Rets.first<F::RetsN>());
  }

protected:
  template <typename SpanA, typename SpanR>
  Expect<void> invoke(const CallingFrame &CallFrame, SpanA &&Args,
                      SpanR &&Rets) {
    using F = FuncTraits<decltype(&T::body)>;
    using ArgsT = typename F::ArgsT;

    auto GeneralArguments = std::tie(*static_cast<T *>(this), CallFrame);
    auto ArgTuple = toTuple<ArgsT>(std::forward<SpanA>(Args),
                                   std::make_index_sequence<F::ArgsN>());
    auto FuncArgTuple =
        std::tuple_cat(std::move(GeneralArguments), std::move(ArgTuple));
    if (auto RetTuple = std::apply(&T::body, std::move(FuncArgTuple))) {
      if constexpr (F::hasReturn) {
        using RetsT = typename F::RetsT;
        fromTuple(std::forward<SpanR>(Rets), RetsT(*RetTuple),
                  std::make_index_sequence<F::RetsN>());
      }
    } else {
      return Unexpect(RetTuple);
    }

    return {};
  }

  void initializeFuncType() {
    auto &FuncType = DefType.getCompositeType().getFuncType();
    using F = FuncTraits<decltype(&T::body)>;
    using ArgsT = typename F::ArgsT;
    FuncType.getParamTypes().reserve(F::ArgsN);
    pushValType<ArgsT>(std::make_index_sequence<F::ArgsN>());
    if constexpr (F::hasReturn) {
      FuncType.getReturnTypes().reserve(F::RetsN);
      using RetsT = typename F::RetsT;
      pushRetType<RetsT>(std::make_index_sequence<F::RetsN>());
    }
  }

private:
  template <typename U> struct Wrap {
    using Type = std::tuple<U>;
  };
  template <typename... U> struct Wrap<std::tuple<U...>> {
    using Type = std::tuple<U...>;
  };
  template <typename> struct FuncTraits;
  template <typename R, typename C, typename... A>
  struct FuncTraits<Expect<R> (C::*)(const CallingFrame &, A...)> {
    using ArgsT = std::tuple<A...>;
    using RetsT = typename Wrap<R>::Type;
    static inline constexpr const std::size_t ArgsN = std::tuple_size_v<ArgsT>;
    static inline constexpr const std::size_t RetsN = std::tuple_size_v<RetsT>;
    static inline constexpr const bool hasReturn = true;
  };
  template <typename C, typename... A>
  struct FuncTraits<Expect<void> (C::*)(const CallingFrame &, A...)> {
    using ArgsT = std::tuple<A...>;
    static inline constexpr const std::size_t ArgsN = std::tuple_size_v<ArgsT>;
    static inline constexpr const std::size_t RetsN = 0;
    static inline constexpr const bool hasReturn = false;
  };

  template <typename Tuple, typename SpanT, size_t... Indices>
  static Tuple toTuple(SpanT &&Args, std::index_sequence<Indices...>) {
    return Tuple(std::forward<SpanT>(Args)[Indices]
                     .template get<std::tuple_element_t<Indices, Tuple>>()...);
  }

  template <typename Tuple, typename SpanT, size_t... Indices>
  static void fromTuple(SpanT &&Rets, Tuple &&V,
                        std::index_sequence<Indices...>) {
    (std::forward<SpanT>(Rets)[Indices]
         .template emplace<std::tuple_element_t<Indices, Tuple>>(
             std::get<Indices>(std::forward<Tuple>(V))),
     ...);
  }

  template <typename Tuple, std::size_t... Indices>
  void pushValType(std::index_sequence<Indices...>) {
    auto &FuncType = DefType.getCompositeType().getFuncType();
    (FuncType.getParamTypes().push_back(
         ValTypeFromType<std::tuple_element_t<Indices, Tuple>>()),
     ...);
  }

  template <typename Tuple, std::size_t... Indices>
  void pushRetType(std::index_sequence<Indices...>) {
    auto &FuncType = DefType.getCompositeType().getFuncType();
    (FuncType.getReturnTypes().push_back(
         ValTypeFromType<std::tuple_element_t<Indices, Tuple>>()),
     ...);
  }
};

} // namespace Runtime
} // namespace WasmEdge
