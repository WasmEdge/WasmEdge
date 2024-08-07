// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "common/spdlog.h"
#include "plugin/plugin.h"
#include <cstdint>
#include <functional>
#include <vector>

#include "chattts.h"
#include "ggml.h"
#include "neuralspeed.h"
#include "onnx.h"
#include "openvino.h"
#include "piper.h"
#include "tf.h"
#include "tfl.h"
#include "torch.h"
#include "types.h"
#include "whispercpp.h"

#ifdef WASMEDGE_BUILD_WASI_NN_RPC
#include <grpc/grpc.h>
#include <grpcpp/create_channel.h>
#endif

namespace WasmEdge {
namespace Host {
namespace WASINN {

namespace detail {
template <typename T, typename V> struct VariantIndex;

template <typename T, typename... Types>
struct VariantIndex<T, std::variant<T, Types...>>
    : std::integral_constant<size_t, 0> {};

template <typename T, typename H, typename... Types>
struct VariantIndex<T, std::variant<H, Types...>>
    : std::integral_constant<
          std::size_t, VariantIndex<T, std::variant<Types...>>::value + 1> {};

template <typename T, typename V>
inline constexpr std::size_t VariantIndexV = VariantIndex<T, V>::value;

template <Backend B> struct BackendTrait;
#define EACH(B)                                                                \
  template <> struct BackendTrait<Backend::B> {                                \
    using Graph = B::Graph;                                                    \
    using Context = B::Context;                                                \
  };
FOR_EACH_BACKEND(EACH)
#undef EACH

template <Backend B> using BackendGraphT = typename BackendTrait<B>::Graph;
template <Backend B> using BackendContextT = typename BackendTrait<B>::Context;
} // namespace detail

class Graph {
public:
  Graph() = delete;
  Graph(Backend BE) noexcept : Impl(std::in_place_type_t<std::monostate>()) {
    switch (BE) {
#define EACH(B)                                                                \
  case Backend::B:                                                             \
    Impl.emplace<B::Graph>();                                                  \
    break;
      FOR_EACH_BACKEND(EACH)
#undef EACH
    default:
      __builtin_unreachable();
    }
  }
  Backend getBackend() const noexcept {
    using V = std::decay_t<decltype(Impl)>;
    switch (Impl.index()) {
#define EACH(B)                                                                \
  case detail::VariantIndexV<B::Graph, V>:                                     \
    return Backend::B;
      FOR_EACH_BACKEND(EACH)
#undef EACH
    default:
      __builtin_unreachable();
    }
  }
  template <Backend B> auto &get() noexcept {
    return *std::get_if<detail::BackendGraphT<B>>(&Impl);
  }
  template <Backend B> const auto &get() const noexcept {
    return *std::get_if<detail::BackendGraphT<B>>(&Impl);
  }
  template <typename T> auto &get() noexcept { return *std::get_if<T>(&Impl); }
  template <typename T> const auto &get() const noexcept {
    return *std::get_if<T>(&Impl);
  }
  std::variant<
#define EACH(B) B::Graph,
      FOR_EACH_BACKEND(EACH)
#undef EACH
          std::monostate>
      Impl;
};

class Context {
public:
  Context() = delete;
  Context(size_t GId, Graph &G) noexcept
      : Impl(std::in_place_type_t<std::monostate>()) {
    switch (G.getBackend()) {
#define EACH(B)                                                                \
  case Backend::B:                                                             \
    Impl.emplace<B::Context>(GId, G.get<Backend::B>());                        \
    break;
      FOR_EACH_BACKEND(EACH)
#undef EACH
    default:
      __builtin_unreachable();
    }
  }

  Backend getBackend() const noexcept {
    using V = std::decay_t<decltype(Impl)>;
    switch (Impl.index()) {
#define EACH(B)                                                                \
  case detail::VariantIndexV<B::Context, V>:                                   \
    return Backend::B;
      FOR_EACH_BACKEND(EACH)
#undef EACH
    default:
      __builtin_unreachable();
    }
  }

  template <Backend B> auto &get() noexcept {
    return *std::get_if<detail::BackendContextT<B>>(&Impl);
  }
  template <Backend B> const auto &get() const noexcept {
    return *std::get_if<detail::BackendContextT<B>>(&Impl);
  }
  template <typename T> auto &get() noexcept { return *std::get_if<T>(&Impl); }
  template <typename T> const auto &get() const noexcept {
    return *std::get_if<T>(&Impl);
  }
  std::variant<
#define EACH(B) B::Context,
      FOR_EACH_BACKEND(EACH)
#undef EACH
          std::monostate>
      Impl;
};

struct WasiNNEnvironment :
#define EACH(B) B::Environ,
    FOR_EACH_BACKEND(EACH)
#undef EACH
        std::monostate {

  using Callback = std::function<Expect<WASINN::ErrNo>(
      WASINN::WasiNNEnvironment &, Span<const Span<uint8_t>>, WASINN::Backend,
      WASINN::Device, uint32_t &)>;

  WasiNNEnvironment() noexcept;

  bool mdGet(std::string Name, uint32_t &GraphId) noexcept {
    std::shared_lock Lock(MdMutex);
    if (auto It = MdMap.find(Name); It != MdMap.end()) {
      GraphId = static_cast<uint32_t>(It->second);
      return true;
    }
    return false;
  }

  void mdRemoveById(uint32_t GraphId) noexcept {
    std::unique_lock Lock(MdMutex);
    for (auto It = MdMap.begin(); It != MdMap.end();) {
      if (It->second == static_cast<uint32_t>(GraphId)) {
        It = MdMap.erase(It);
      } else {
        ++It;
      }
    }
  }

  Expect<WASINN::ErrNo>
  mdBuild(std::string Name, uint32_t &GraphId, Callback Load,
          std::vector<uint8_t> Config = std::vector<uint8_t>()) noexcept {
    std::unique_lock Lock(MdMutex);
    auto It = RawMdMap.find(Name);
    if (It != RawMdMap.end()) {
      auto RawMd = std::get<0>(It->second);
      std::vector<Span<uint8_t>> Builders;
      Builders.reserve(RawMd.size());
      for (auto &Builder : RawMd) {
        Builders.emplace_back(Builder);
      }
      // Add config to the end of Builders if exists.
      if (Config.size() > 0) {
        Builders.emplace_back(Config);
      }
      auto Result = Load(*this, Builders, std::get<1>(It->second),
                         std::get<2>(It->second), GraphId);
      if (Result.has_value()) {
        MdMap[Name] = GraphId;
      }
      return Result;
    }
    return WASINN::ErrNo::NotFound;
  }

  mutable std::shared_mutex MdMutex; ///< Protect MdMap
  std::unordered_map<std::string, std::tuple<std::vector<std::vector<uint8_t>>,
                                             Backend, Device>>
      RawMdMap;
  std::unordered_map<std::string, uint32_t> MdMap;
  std::vector<Graph> NNGraph;
  std::vector<Context> NNContext;
  static PO::List<std::string> NNModels;
#ifdef WASMEDGE_BUILD_WASI_NN_RPC
  static PO::Option<std::string> NNRPCURI; // For RPC client mode
  std::shared_ptr<grpc::Channel> NNRPCChannel;
#endif
};

} // namespace WASINN
} // namespace Host
} // namespace WasmEdge
