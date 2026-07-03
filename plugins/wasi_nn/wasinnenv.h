// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "GGML/core/ggml_core.h"
#include "resource_table.h"
#include "wasinn_bitnet.h"
#include "wasinn_chattts.h"
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

#include "host/wasi/environ.h"

#include "common/spdlog.h"
#include "host/wasi/wasimodule.h"
#include "plugin/plugin.h"
#include "runtime/callingframe.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
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

// Graph lifecycle status. A Graph is published to the handle table only fully
// loaded, so a half-built graph is never observable.
//   Ready:    Can create contexts and run inference.
//   Invalid:  A set_input metadata reload failed; reloadable in set_input.
//   Detached: Unloaded by the guest. The handle is dead; earlier contexts
//             keep the object alive to drain. Terminal, so a reload racing
//             the unload cannot resurrect the graph.
enum class GraphStatus : uint8_t { Ready, Invalid, Detached };

// How strict a host op's owning-graph status check is: Any (just resolve the
// handle), NotDetached (Invalid admitted for reload), Ready (fully usable),
// or Drainable (Ready or Detached: may read the model after unload, while a
// model-less Invalid graph is rejected).
enum class GraphReq : uint8_t { Any, NotDetached, Ready, Drainable };

constexpr bool graphAdmits(GraphReq Req, GraphStatus Stat) noexcept {
  switch (Req) {
  case GraphReq::Any:
    return true;
  case GraphReq::NotDetached:
    return Stat != GraphStatus::Detached;
  case GraphReq::Ready:
    return Stat == GraphStatus::Ready;
  case GraphReq::Drainable:
    return Stat == GraphStatus::Ready || Stat == GraphStatus::Detached;
  }
  return false;
}

// Wrapper owning one backend graph, shared_ptr-managed through the handle
// table: host ops and child contexts pin it; the payload (and model) dies at
// the last release. The payload variant is emplaced once and never reset, so
// the typed accessors cannot observe a mismatched alternative.
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

  GraphStatus status() const noexcept {
    return Stat.load(std::memory_order_acquire);
  }
  bool isReady() const noexcept { return status() == GraphStatus::Ready; }
  // unload writes Detached without the op mutex, so it never blocks behind a
  // running op; a (re)load finishing later must not resurrect the graph, so
  // these transitions CAS and give up once they observe Detached.
  void setReady() noexcept { transitionUnlessDetached(GraphStatus::Ready); }
  void setInvalid() noexcept { transitionUnlessDetached(GraphStatus::Invalid); }
  void setDetached() noexcept {
    Stat.store(GraphStatus::Detached, std::memory_order_release);
  }

  void setModelName(std::string Name) noexcept { ModelName = std::move(Name); }
  const std::string &getModelName() const noexcept { return ModelName; }

  std::mutex &opMutex() noexcept { return OpMutex; }

private:
  void transitionUnlessDetached(GraphStatus Next) noexcept {
    auto Cur = Stat.load(std::memory_order_acquire);
    while (Cur != GraphStatus::Detached &&
           !Stat.compare_exchange_weak(Cur, Next, std::memory_order_acq_rel,
                                       std::memory_order_acquire)) {
    }
  }

  std::variant<
#define EACH(B) B::Graph,
      FOR_EACH_BACKEND(EACH)
#undef EACH
          std::monostate>
      Impl;
  std::atomic<GraphStatus> Stat{GraphStatus::Ready};
  // The nn-preload name; read on unload to evict the name cache entry.
  std::string ModelName;
  // Serializes backend ops on this graph; shared by its contexts because
  // backends keep mutable inference state on the graph payload.
  std::mutex OpMutex;
};

// Wrapper owning one backend execution context plus a pin on its parent
// graph, so a context cannot outlive its graph: that is what lets a live
// context drain after the guest unloads the graph.
class Context {
public:
  Context() = delete;
  Context(uint32_t GId, std::shared_ptr<Graph> G) noexcept
      : Parent(std::move(G)), Impl(std::in_place_type_t<std::monostate>()),
        GraphId(GId) {
    switch (Parent->getBackend()) {
#define EACH(B)                                                                \
  case Backend::B:                                                             \
    Impl.emplace<B::Context>(GId, Parent->get<Backend::B>());                  \
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

  Graph &parent() noexcept { return *Parent; }
  const Graph &parent() const noexcept { return *Parent; }
  uint32_t getGraphId() const noexcept { return GraphId; }

private:
  std::shared_ptr<Graph> Parent;
  std::variant<
#define EACH(B) B::Context,
      FOR_EACH_BACKEND(EACH)
#undef EACH
          std::monostate>
      Impl;
  uint32_t GraphId;
};

// Every graph/context-resolving host op declares its GraphReq tier and
// error-log label in hostOpPolicy(). One table keeps the whole liveness
// contract in view, so a wrong tier stands out against its siblings and a
// new op must make a deliberate choice.
enum class HostOp : uint8_t {
  InitExecCtx,
  SetInput,
  Compute,
  ComputeSingle,
  GetOutput,
  GetOutputSingle,
  FiniSingle,
};

struct HostOpPolicy {
  GraphReq Req;
  std::string_view Name;
};

constexpr HostOpPolicy hostOpPolicy(HostOp Op) noexcept {
  using namespace std::literals;
  switch (Op) {
  case HostOp::InitExecCtx:
    return {GraphReq::Ready, "init_execution_context"sv};
  case HostOp::SetInput:
    return {GraphReq::NotDetached, "set_input"sv};
  case HostOp::Compute:
    return {GraphReq::Ready, "compute"sv};
  case HostOp::ComputeSingle:
    return {GraphReq::Ready, "compute_single"sv};
  case HostOp::GetOutput:
    return {GraphReq::Any, "get_output"sv};
  case HostOp::GetOutputSingle:
    return {GraphReq::Drainable, "get_output_single"sv};
  case HostOp::FiniSingle:
    return {GraphReq::Any, "fini_single"sv};
  }
  return {GraphReq::Ready, ""sv};
}

struct WasiNNEnvironment :
#define EACH(B) B::Environ,
    FOR_EACH_BACKEND(EACH)
#undef EACH
        std::monostate {

  using Callback = std::function<Expect<WASINN::ErrNo>(
      WASINN::WasiNNEnvironment &, Span<const Span<uint8_t>>, WASINN::Backend,
      WASINN::Device, std::string_view, uint32_t &)>;

  WasiNNEnvironment() noexcept;

  // Resolve a preloaded model name to a live graph handle. Handles are never
  // reused, so a cache entry whose graph was unloaded simply stops resolving.
  bool mdGet(std::string_view Name, uint32_t &GraphId) noexcept {
    std::shared_lock Lock(MdMutex);
    if (auto It = MdMap.find(std::string(Name)); It != MdMap.end()) {
      if (NNGraph.get(It->second) != nullptr) {
        GraphId = It->second;
        return true;
      }
    }
    return false;
  }

  // Build a graph from the preloaded model bytes registered under Name.
  // MdMutex guards only the name -> handle map and is never held across the
  // (possibly minutes-long) model load; RawMdMap is constructor-immutable.
  Expect<WASINN::ErrNo>
  mdBuild(const std::string &Name, uint32_t &GraphId, Callback Load,
          std::vector<uint8_t> Config = std::vector<uint8_t>()) noexcept {
    auto It = RawMdMap.find(Name);
    if (It == RawMdMap.end()) {
      return WASINN::ErrNo::NotFound;
    }
    auto &RawMd = std::get<0>(It->second);
    std::vector<Span<uint8_t>> Builders;
    Builders.reserve(RawMd.size() + 1);
    for (auto &Builder : RawMd) {
      Builders.emplace_back(Builder);
    }
    // Add config to the end of Builders if exists.
    if (Config.size() > 0) {
      Builders.emplace_back(Config);
    }
    auto Result = Load(*this, Builders, std::get<1>(It->second),
                       std::get<2>(It->second), Name, GraphId);
    if (Result.has_value() && *Result == WASINN::ErrNo::Success) {
      // Concurrent builds for the same name race here: each ran outside the
      // lock and published its own graph. Keep the first cached live graph
      // and fold later finishers onto it, dropping their duplicate builds,
      // so one name never retains two copies of a model.
      std::unique_lock Lock(MdMutex);
      if (auto Cached = MdMap.find(Name);
          Cached != MdMap.end() && NNGraph.get(Cached->second) != nullptr) {
        const uint32_t LoserId = GraphId;
        GraphId = Cached->second;
        Lock.unlock();
        if (auto Loser = NNGraph.remove(LoserId); Loser != nullptr) {
          Loser->setDetached();
        }
        return Result;
      }
      MdMap[Name] = GraphId;
    }
    return Result;
  }

  // Uniform graph unload: detach the handle now and let the backend payload
  // destructor run at the last release - immediately if nothing pins the
  // graph, or after the last draining context / in-flight op otherwise. The
  // host call never blocks behind a running compute.
  Expect<WASINN::ErrNo> unloadGraph(uint32_t GraphId) noexcept {
    using namespace std::literals;
    auto G = NNGraph.remove(GraphId);
    if (G == nullptr) {
      spdlog::error(
          "[WASI-NN] unload: Graph ID {} does not exist or is unloaded."sv,
          GraphId);
      return WASINN::ErrNo::InvalidArgument;
    }
    G->setDetached();
    // Evict the name cache so a later load_by_name reloads instead of
    // resolving to the dead handle; matching the cached id is exact.
    if (const auto &Name = G->getModelName(); !Name.empty()) {
      std::unique_lock Lock(MdMutex);
      if (auto It = MdMap.find(Name);
          It != MdMap.end() && It->second == GraphId) {
        MdMap.erase(It);
      }
    }
    return WASINN::ErrNo::Success;
  }

  // Detach the handle; the destructor drops backend state and the graph pin.
  Expect<WASINN::ErrNo> finalizeContext(uint32_t ContextId) noexcept {
    using namespace std::literals;
    if (NNContext.remove(ContextId) == nullptr) {
      spdlog::error("[WASI-NN] finalize_execution_context: Context ID {} "
                    "does not exist."sv,
                    ContextId);
      return WASINN::ErrNo::InvalidArgument;
    }
    return WASINN::ErrNo::Success;
  }

  // Resolve and pin the graph, serialize on its op mutex, then re-check the
  // status (an unload or a failed reload may have won the race) before
  // running Dispatch. The pin keeps the graph alive for the whole op.
  template <typename Fn>
  Expect<WASINN::ErrNo> withGraphOp(uint32_t GraphId, HostOp Op,
                                    Fn &&Dispatch) noexcept {
    const auto Policy = hostOpPolicy(Op);
    auto G = NNGraph.get(GraphId);
    if (G == nullptr || !graphAdmits(Policy.Req, G->status())) {
      logGraphReject(Policy.Name, GraphId, G.get());
      return WASINN::ErrNo::InvalidArgument;
    }
    std::lock_guard<std::mutex> OpLock(G->opMutex());
    if (!graphAdmits(Policy.Req, G->status())) {
      logGraphReject(Policy.Name, GraphId, G.get());
      return WASINN::ErrNo::InvalidArgument;
    }
    return Dispatch(G);
  }

  // Context-keyed twin of withGraphOp: the context pin keeps the context and
  // parent graph alive; the parent's op mutex serializes against every other
  // op on the same graph.
  template <typename Fn>
  Expect<WASINN::ErrNo> withContextOp(uint32_t ContextId, HostOp Op,
                                      Fn &&Dispatch) noexcept {
    using namespace std::literals;
    const auto Policy = hostOpPolicy(Op);
    auto C = NNContext.get(ContextId);
    if (C == nullptr) {
      spdlog::error("[WASI-NN] {}: Context ID {} does not exist."sv,
                    Policy.Name, ContextId);
      return WASINN::ErrNo::InvalidArgument;
    }
    Graph &G = C->parent();
    if (!graphAdmits(Policy.Req, G.status())) {
      logContextGraphReject(Policy.Name, ContextId, *C);
      return WASINN::ErrNo::InvalidArgument;
    }
    std::lock_guard<std::mutex> OpLock(G.opMutex());
    if (!graphAdmits(Policy.Req, G.status())) {
      logContextGraphReject(Policy.Name, ContextId, *C);
      return WASINN::ErrNo::InvalidArgument;
    }
    return Dispatch(G, *C);
  }

  // Md storage. RawMdMap is immutable after the constructor; MdMutex guards
  // MdMap only. Lock order: MdMutex before the handle-table mutex (mdGet).
  mutable std::shared_mutex MdMutex;
  std::unordered_map<std::string, std::tuple<std::vector<std::vector<uint8_t>>,
                                             Backend, Device>>
      RawMdMap;
  std::unordered_map<std::string, uint32_t> MdMap;

  // Graph and context handle tables. Declared in this order so contexts are
  // destroyed before graphs on environment teardown.
  ResourceTable<Graph> NNGraph;
  ResourceTable<Context> NNContext;

  // Preload model list
  static PO::List<std::string> NNModels;
#ifdef WASMEDGE_BUILD_WASI_NN_RPC
  static PO::Option<std::string> NNRPCURI; // For RPC client mode
  std::shared_ptr<grpc::Channel> NNRPCChannel;
#endif

  const Host::WASI::Environ *getEnv() const noexcept {
    return Environ.load(std::memory_order_acquire);
  }
  // Concurrent host calls re-derive the same environ; the atomic store makes
  // that race benign instead of undefined behavior.
  void setEnviron(const Runtime::CallingFrame *CurrentFrame) noexcept {
    auto *WasiModule = CurrentFrame->getWASIModule();
    if (WasiModule != nullptr) {
      Environ.store(dynamic_cast<const WasmEdge::Host::WasiModule *>(WasiModule)
                        ->getEnv(),
                    std::memory_order_release);
    }
  }

private:
  void logGraphReject(std::string_view Op, uint32_t GraphId,
                      const Graph *G) const noexcept {
    using namespace std::literals;
    if (G != nullptr && G->status() == GraphStatus::Invalid) {
      spdlog::error("[WASI-NN] {}: Graph ID {} is invalid. Please reload or "
                    "unload this graph."sv,
                    Op, GraphId);
    } else {
      spdlog::error(
          "[WASI-NN] {}: Graph ID {} does not exist or is unloaded."sv, Op,
          GraphId);
    }
  }
  void logContextGraphReject(std::string_view Op, uint32_t ContextId,
                             const Context &C) const noexcept {
    using namespace std::literals;
    spdlog::error("[WASI-NN] {}: Graph ID {} for context ID {} does not "
                  "exist or has released."sv,
                  Op, C.getGraphId(), ContextId);
  }

  std::atomic<const Host::WASI::Environ *> Environ{nullptr};
};

} // namespace WASINN
} // namespace Host
} // namespace WasmEdge
