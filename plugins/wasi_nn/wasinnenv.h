// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "wasinn_bitnet.h"
#include "wasinn_chattts.h"
#include "wasinn_ggml.h"
#include "wasinn_mlx.h"
#include "wasinn_neuralspeed.h"
#include "wasinn_onnx.h"
#include "wasinn_openvino.h"
#include "wasinn_openvino_genai.h"
#include "wasinn_piper.h"
#include "wasinn_tf.h"
#include "wasinn_tfl.h"
#include "wasinn_torch.h"
#include "wasinn_whisper.h"
#include "wasinntypes.h"

#include "common/spdlog.h"
#include "plugin/plugin.h"

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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
    init(BE);
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

  void init(Backend BE) noexcept {
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
    Stat = Status::Uninitialized;
    CtxCnt = 0;
  }
  void reset() noexcept {
    Impl = std::monostate{};
    Stat = Status::Uninitialized;
    CtxCnt = 0;
  }
  void increaseContext() noexcept { CtxCnt++; }
  void decreaseContext() noexcept {
    assuming(CtxCnt > 0);
    CtxCnt--;
  }
  uint32_t getContextCount() const noexcept { return CtxCnt; }
  bool isFinalized() const noexcept {
    return Stat == Status::Uninitialized || Stat == Status::Finalized;
  }
  bool isReady() const noexcept { return Stat == Status::Ready; }
  void setInvalid() noexcept { Stat = Status::Invalid; }
  void setFinalized() noexcept { Stat = Status::Finalized; }
  void setReady() noexcept { Stat = Status::Ready; }

private:
  std::variant<
#define EACH(B) B::Graph,
      FOR_EACH_BACKEND(EACH)
#undef EACH
          std::monostate>
      Impl;
  // Graph status.
  //   Uninitialized: A new graph in monostate.
  //   Invalid: The graph loaded failed in set_input with metadata. Can be
  //            reload with a new metadata in set_input.
  //   Finalized: The graph being deleted, but there are contexts linked. This
  //              graph ID will be released once the contexts are deleted.
  //   Ready: This graph can be used to create a context.
  enum class Status : uint8_t { Uninitialized, Invalid, Finalized, Ready };
  Status Stat;
  uint32_t CtxCnt;
};

class Context {
public:
  Context() = delete;
  Context(uint32_t GId, Graph &G) noexcept
      : Impl(std::in_place_type_t<std::monostate>()) {
    init(GId, G);
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

  void init(uint32_t GId, Graph &G) noexcept {
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
    Stat = Status::Uninitialized;
    GraphId = GId;
  }
  void reset() noexcept {
    Impl = std::monostate{};
    Stat = Status::Uninitialized;
    GraphId = 0;
  }
  uint32_t getGraphId() const noexcept {
    return static_cast<uint32_t>(GraphId);
  }
  bool isReady() const noexcept { return Stat == Status::Ready; }
  void setReady() noexcept { Stat = Status::Ready; }

private:
  std::variant<
#define EACH(B) B::Context,
      FOR_EACH_BACKEND(EACH)
#undef EACH
          std::monostate>
      Impl;
  // Context status.
  //   Uninitialized: A new context in monostate.
  //   Ready: This context can be used to infer.
  enum class Status : uint8_t { Uninitialized, Ready };
  Status Stat;
  uint32_t GraphId;
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
      GraphId = EndianValue(static_cast<uint32_t>(It->second)).le();
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

  uint32_t newGraph(Backend BE) noexcept {
    std::unique_lock Lock(GraphMutex);
    uint32_t ID = static_cast<uint32_t>(NNGraph.size());
    if (NNGraphRecycle.empty()) {
      NNGraph.emplace_back(BE);
    } else {
      ID = *NNGraphRecycle.begin();
      NNGraph[ID].init(BE);
      NNGraphRecycle.erase(ID);
    }
    return ID;
  }

  uint32_t newContext(uint32_t GId, Graph &G) noexcept {
    std::unique_lock Lock(GraphMutex);
    assuming(NNGraph.size() > GId);
    // TODO: Merge GId into graph class.
    uint32_t ID = static_cast<uint32_t>(NNContext.size());
    if (NNContextRecycle.empty()) {
      NNContext.emplace_back(GId, G);
    } else {
      ID = *NNContextRecycle.begin();
      NNContext[ID].init(GId, G);
      NNContextRecycle.erase(ID);
    }
    G.increaseContext();
    return ID;
  }

  void deleteGraph(const uint32_t Id) noexcept {
    // TODO: Add the deallocation callback.
    std::unique_lock Lock(GraphMutex);
    if (Id < NNGraph.size()) {
      auto &G = NNGraph[Id];
      G.setFinalized();
      if (G.getContextCount() == 0) {
        // Checked all contexts are deleted. Release the graph id.
        if (Id == NNGraph.size() - 1) {
          NNGraph.pop_back();
        } else {
          G.reset();
          NNGraphRecycle.insert(Id);
        }
      }
    }
  }

  void deleteContext(const uint32_t Id) noexcept {
    // TODO: Add the deallocation callback.
    std::unique_lock Lock(GraphMutex);
    if (Id < NNContext.size() &&
        NNContextRecycle.find(Id) == NNContextRecycle.end()) {
      auto GId = NNContext[Id].getGraphId();
      auto &G = NNGraph[GId];
      G.decreaseContext();
      if (G.getContextCount() == 0 && G.isFinalized()) {
        // Checked all contexts are deleted. Release the graph id.
        if (GId == NNGraph.size() - 1) {
          NNGraph.pop_back();
        } else {
          G.reset();
          NNGraphRecycle.insert(GId);
        }
      }
      if (Id == NNContext.size() - 1) {
        NNContext.pop_back();
      } else {
        NNContext[Id].reset();
        NNContextRecycle.insert(Id);
      }
    }
  }

  // Md storage
  mutable std::shared_mutex MdMutex;
  std::unordered_map<std::string, std::tuple<std::vector<std::vector<uint8_t>>,
                                             Backend, Device>>
      RawMdMap;
  std::unordered_map<std::string, uint32_t> MdMap;

  // Graph and context
  mutable std::shared_mutex GraphMutex;
  std::unordered_set<uint32_t> NNGraphRecycle;
  std::vector<Graph> NNGraph;
  std::unordered_set<uint32_t> NNContextRecycle;
  std::vector<Context> NNContext;

  // Preload model list
  static PO::List<std::string> NNModels;
#ifdef WASMEDGE_BUILD_WASI_NN_RPC
  static PO::Option<std::string> NNRPCURI; // For RPC client mode
  std::shared_ptr<grpc::Channel> NNRPCChannel;
#endif
};

} // namespace WASINN
} // namespace Host
} // namespace WasmEdge
