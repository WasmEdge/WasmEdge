// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#pragma once

#include "ast/instruction.h"
#include "common/symbol.h"
#include "common/types.h"

#include <memory>
#include <numeric>
#include <string>
#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Component {

class HostFunctionBase {
public:
  HostFunctionBase() : FuncType{AST::FunctionType()} {}
  virtual ~HostFunctionBase() = default;

  /// Run host function body.
  virtual Expect<void> run(Span<const ValInterface> Args,
                           Span<ValInterface> Rets) = 0;

  /// Getter of function type.
  const AST::FunctionType &getFuncType() const noexcept { return FuncType; }
  AST::FunctionType &getFuncType() noexcept { return FuncType; }

protected:
  AST::FunctionType FuncType;
};

template <typename ArgT> struct convert {
  static ArgT run(const ValInterface &V) {
    return std::get<ValVariant>(V).template get<ArgT>();
  }
};
template <> struct convert<std::string> {
  static std::string run(const ValInterface &V) {
    return std::get<std::string>(V);
  }
};
template <typename T> struct convert<List<T>> {
  static List<T> run(const ValInterface &V) { return std::get<List<T>>(V); }
};

template <typename T> class HostFunction : public HostFunctionBase {
public:
  HostFunction() : HostFunctionBase() { initializeFuncType(); }

  Expect<void> run(Span<const ValInterface> Args,
                   Span<ValInterface> Rets) override {
    using F = FuncTraits<decltype(&T::body)>;
    if (unlikely(F::ArgsN != Args.size())) {
      return Unexpect(ErrCode::Value::FuncSigMismatch);
    }
    if (unlikely(F::RetsN != Rets.size())) {
      return Unexpect(ErrCode::Value::FuncSigMismatch);
    }
    return invoke(Args.first<F::ArgsN>(), Rets.first<F::RetsN>());
  }

protected:
  template <typename SpanA, typename SpanR>
  Expect<void> invoke(SpanA &&Args, SpanR &&Rets) {
    using F = FuncTraits<decltype(&T::body)>;
    using ArgsT = typename F::ArgsT;

    auto GeneralArguments = std::tie(*static_cast<T *>(this));
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
    auto &FuncType = getFuncType();
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
  struct FuncTraits<Expect<R> (C::*)(A...)> {
    using ArgsT = std::tuple<A...>;
    using RetsT = typename Wrap<R>::Type;
    static inline constexpr const std::size_t ArgsN = std::tuple_size_v<ArgsT>;
    static inline constexpr const std::size_t RetsN = std::tuple_size_v<RetsT>;
    static inline constexpr const bool hasReturn = true;
  };
  template <typename C, typename... A>
  struct FuncTraits<Expect<void> (C::*)(A...)> {
    using ArgsT = std::tuple<A...>;
    static inline constexpr const std::size_t ArgsN = std::tuple_size_v<ArgsT>;
    static inline constexpr const std::size_t RetsN = 0;
    static inline constexpr const bool hasReturn = false;
  };

  template <typename Tuple, typename SpanT, size_t... Indices>
  static Tuple toTuple(SpanT &&Args, std::index_sequence<Indices...>) {
    return Tuple(convert<std::tuple_element_t<Indices, Tuple>>::run(
        std::forward<SpanT>(Args)[Indices])...);
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
    (FuncType.getParamTypes().push_back(
         Wit<std::tuple_element_t<Indices, Tuple>>::type()),
     ...);
  }

  template <typename Tuple, std::size_t... Indices>
  void pushRetType(std::index_sequence<Indices...>) {
    (FuncType.getReturnTypes().push_back(
         Wit<std::tuple_element_t<Indices, Tuple>>::type()),
     ...);
  }
};

} // namespace Component
} // namespace Runtime
} // namespace WasmEdge
