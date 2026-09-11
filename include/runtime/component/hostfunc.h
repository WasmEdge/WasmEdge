// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/component/hostfunc.h - Host function interface ---===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the interface of the host component function class: a
/// function the host implements over component-level values.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/type.h"
#include "common/component_variant.h"
#include "common/errcode.h"
#include "common/span.h"
#include "runtime/component/wit.h"

#include <cstddef>
#include <iterator>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Component {

class CallingFrame;

/// A host-implemented component function. It runs on component-level values
/// and owns the component function type it declares.
class HostFunctionBase {
public:
  HostFunctionBase() = default;
  virtual ~HostFunctionBase() = default;

  /// Run host function body.
  virtual Expect<void> run(CallingFrame &Frame,
                           Span<const ComponentValVariant> Args,
                           Span<ComponentValVariant> Rets) = 0;

  /// Declare the function type, minting the indices of its defined types
  /// through the minter. The owning instance calls this once at registration.
  virtual void declare(const TypeMinter &) noexcept {}

  /// Getter for function type.
  const AST::Component::FuncType &getFuncType() const noexcept {
    return DeclType;
  }

protected:
  AST::Component::FuncType DeclType;
};

/// A host function over a typed body `Expect<R> T::body(CallingFrame &, A...)`.
/// The function type derives from the body through Wit<>, and T names its
/// parameters in `static constexpr const char *ParamNames[]`.
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
    std::vector<AST::Component::LabelValType> Results;
    if constexpr (F::HasResult) {
      Results.emplace_back(Wit<typename F::RetT>::type(Mint));
    }
    DeclType = AST::Component::FuncType(std::move(Params), std::move(Results));
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

} // namespace Component
} // namespace Runtime
} // namespace WasmEdge
