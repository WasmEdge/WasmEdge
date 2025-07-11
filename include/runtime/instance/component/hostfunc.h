// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#pragma once

#include "ast/type.h"
#include "common/symbol.h"
#include "common/types.h"

#include <memory>
#include <numeric>
#include <string>

namespace WasmEdge {
namespace Runtime {
namespace Instance {
namespace Component {

class HostFunctionBase {
public:
  HostFunctionBase() = default;
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
template <> struct convert<bool> {
  static bool run(const ValInterface &V) { return std::get<bool>(V); }
};
template <> struct convert<uint8_t> {
  static uint8_t run(const ValInterface &V) { return std::get<uint8_t>(V); }
};
template <> struct convert<uint16_t> {
  static uint16_t run(const ValInterface &V) { return std::get<uint16_t>(V); }
};
template <> struct convert<uint32_t> {
  static uint32_t run(const ValInterface &V) { return std::get<uint32_t>(V); }
};
template <> struct convert<uint64_t> {
  static uint64_t run(const ValInterface &V) { return std::get<uint64_t>(V); }
};
template <> struct convert<int8_t> {
  static int8_t run(const ValInterface &V) { return std::get<int8_t>(V); }
};
template <> struct convert<int16_t> {
  static int16_t run(const ValInterface &V) { return std::get<int16_t>(V); }
};

template <> struct convert<std::string> {
  static std::string run(const ValInterface &V) {
    return std::get<std::string>(V);
  }
};
template <typename T> struct convert<List<T>> {
  static List<T> run(const ValInterface &V) {
    auto *C = std::get<std::shared_ptr<ValComp>>(V).get();
    return *dynamic_cast<List<T> *>(C);
  }
};
template <typename... Types> struct convert<Record<Types...>> {
  static Record<Types...> run(const ValInterface &V) {
    auto *C = std::get<std::shared_ptr<ValComp>>(V).get();
    return *dynamic_cast<Record<Types...> *>(C);
  }
};
template <typename... Types> struct convert<Tuple<Types...>> {
  static Tuple<Types...> run(const ValInterface &V) {
    auto *C = std::get<std::shared_ptr<ValComp>>(V).get();
    return *dynamic_cast<Tuple<Types...> *>(C);
  }
};
template <typename T> struct convert<Option<T>> {
  static Option<T> run(const ValInterface &V) {
    auto *C = std::get<std::shared_ptr<ValComp>>(V).get();
    return *dynamic_cast<Option<T> *>(C);
  }
};
template <> struct convert<Enum> {
  static Enum run(const ValInterface &V) {
    auto *C = std::get<std::shared_ptr<ValComp>>(V).get();
    return *dynamic_cast<Enum *>(C);
  }
};
template <typename V, typename E> struct convert<Result<V, E>> {
  static Result<V, E> run(const ValInterface &Val) {
    auto *C = std::get<std::shared_ptr<ValComp>>(Val).get();
    return *dynamic_cast<Result<V, E> *>(C);
  }
};

template <typename ArgT> struct emplace {
  static void run(ValInterface &V, ArgT Arg) {
    std::get<ValVariant>(V).emplace<ArgT>(Arg);
  }
};
template <> struct emplace<bool> {
  static void run(ValInterface &V, bool Arg) { V.emplace<bool>(Arg); }
};
template <> struct emplace<uint8_t> {
  static void run(ValInterface &V, uint8_t Arg) { V.emplace<uint8_t>(Arg); }
};
template <> struct emplace<uint16_t> {
  static void run(ValInterface &V, uint16_t Arg) { V.emplace<uint16_t>(Arg); }
};
template <> struct emplace<uint32_t> {
  static void run(ValInterface &V, uint32_t Arg) { V.emplace<uint32_t>(Arg); }
};
template <> struct emplace<uint64_t> {
  static void run(ValInterface &V, uint64_t Arg) { V.emplace<uint64_t>(Arg); }
};
template <> struct emplace<int8_t> {
  static void run(ValInterface &V, int8_t Arg) { V.emplace<int8_t>(Arg); }
};
template <> struct emplace<int16_t> {
  static void run(ValInterface &V, int16_t Arg) { V.emplace<int16_t>(Arg); }
};
template <> struct emplace<std::string> {
  static void run(ValInterface &V, std::string Arg) {
    V.emplace<std::string>(Arg);
  }
};
template <typename T> struct emplace<List<T>> {
  static void run(ValInterface &V, List<T> Arg) {
    V.emplace<std::shared_ptr<ValComp>>(std::make_shared<List<T>>(Arg));
  }
};
template <typename... Types> struct emplace<Record<Types...>> {
  static void run(ValInterface &V, Record<Types...> Arg) {
    V.emplace<std::shared_ptr<ValComp>>(
        std::make_shared<Record<Types...>>(Arg));
  }
};
template <typename... Types> struct emplace<Tuple<Types...>> {
  static void run(ValInterface &V, Tuple<Types...> Arg) {
    V.emplace<std::shared_ptr<ValComp>>(std::make_shared<Tuple<Types...>>(Arg));
  }
};
template <typename T> struct emplace<Option<T>> {
  static void run(ValInterface &V, Option<T> Arg) {
    V.emplace<std::shared_ptr<ValComp>>(std::make_shared<Option<T>>(Arg));
  }
};
template <> struct emplace<Enum> {
  static void run(ValInterface &V, Enum Arg) {
    V.emplace<std::shared_ptr<ValComp>>(std::make_shared<Enum>(Arg));
  }
};
template <typename V, typename E> struct emplace<Result<V, E>> {
  static void run(ValInterface &Val, Result<V, E> Arg) {
    Val.emplace<std::shared_ptr<ValComp>>(std::make_shared<Result<V, E>>(Arg));
  }
};
template <typename... Types>
struct emplace<WasmEdge::Component::Variant<Types...>> {
  static void run(ValInterface &V, WasmEdge::Component::Variant<Types...> Arg) {
    V.emplace<std::shared_ptr<ValComp>>(
        std::make_shared<WasmEdge::Component::Variant<Types...>>(Arg));
  }
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
    if constexpr (F::hasReturn) {
      EXPECTED_TRY(typename F::RetsT RetTuple,
                   std::apply(&T::body, std::move(FuncArgTuple)));
      fromTuple(std::forward<SpanR>(Rets), std::move(RetTuple),
                std::make_index_sequence<F::RetsN>());
    } else {
      EXPECTED_TRY(std::apply(&T::body, std::move(FuncArgTuple)));
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
    (emplace<std::tuple_element_t<Indices, Tuple>>::run(
         std::forward<SpanT>(Rets)[Indices],
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
} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
