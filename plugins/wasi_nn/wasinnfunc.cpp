// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "wasinnfunc.h"
#include "wasinnenv.h"

#include "common/spdlog.h"

#include <string>
#include <string_view>

#ifdef WASMEDGE_BUILD_WASI_NN_RPC
#include "wasi_ephemeral_nn.grpc.pb.h"

#include <charconv>
#include <grpc/grpc.h>
#endif // #ifdef WASMEDGE_BUILD_WASI_NN_RPC

namespace WasmEdge {
namespace Host {

namespace {
inline void reportUnknownBackend(WASINN::Backend B) noexcept {
  spdlog::error("[WASI-NN] Unknown backend {}."sv, static_cast<uint32_t>(B));
}
Expect<WASINN::ErrNo> load(WASINN::WasiNNEnvironment &Env,
                           Span<const Span<uint8_t>> Builders,
                           WASINN::Backend Backend, WASINN::Device Device,
                           std::string_view Name, uint32_t &GraphId) {
  // Build the graph fully before publishing: a table entry is always a
  // usable resource and a failed load leaves no trace.
  auto LoadInto = [&](WASINN::Backend BE,
                      auto BackendLoad) -> Expect<WASINN::ErrNo> {
    auto G = std::make_shared<WASINN::Graph>(BE);
    G->setModelName(std::string(Name));
    auto Res = BackendLoad(Env, *G, Builders, Device);
    if (!Res.has_value() || *Res != WASINN::ErrNo::Success) {
      return Res;
    }
    const auto Id = Env.NNGraph.insert(std::move(G));
    if (!Id.has_value()) {
      spdlog::error("[WASI-NN] load: graph handle space is exhausted."sv);
      return WASINN::ErrNo::RuntimeError;
    }
    GraphId = *Id;
    return WASINN::ErrNo::Success;
  };
  switch (Backend) {
#define EACH(B)                                                                \
  case WASINN::Backend::B:                                                     \
    return LoadInto(WASINN::Backend::B,                                        \
                    [](auto &...Args) { return WASINN::B::load(Args...); });
    FOR_EACH_BACKEND(EACH)
#undef EACH
  default:
    reportUnknownBackend(Backend);
    return WASINN::ErrNo::InvalidEncoding;
  }
}
#ifdef WASMEDGE_BUILD_WASI_NN_RPC
WASINN::ErrNo metadataToErrNo(
    const std::multimap<grpc::string_ref, grpc::string_ref> &Metadata) {
  const auto It = Metadata.find("errno");
  if (It != Metadata.end()) {
    const grpc::string_ref &ErrNoRef = It->second;
    int ErrNoValue = 0;
    const char *Begin = ErrNoRef.data();
    const auto Result =
        std::from_chars(Begin, Begin + ErrNoRef.size(), ErrNoValue);
    if (Result.ec == std::errc()) {
      return static_cast<WASINN::ErrNo>(ErrNoValue);
    }
  }
  return WASINN::ErrNo::RuntimeError;
}
#endif // #ifdef WASMEDGE_BUILD_WASI_NN_RPC
} // namespace

Expect<WASINN::ErrNo>
WasiNNLoad::bodyImpl(const Runtime::CallingFrame &Frame, uint32_t BuilderPtr,
                     uint32_t BuilderLen, uint32_t RawEncoding, uint32_t Target,
                     uint32_t GraphIdPtr) {
  Env.setEnviron(&Frame);
#ifdef WASMEDGE_BUILD_WASI_NN_RPC
  if (Env.NNRPCChannel != nullptr) {
    // TODO: implement RPC for Load
    spdlog::error("[WASI-NN] RPC client is not implemented for Load"sv);
    return WASINN::ErrNo::UnsupportedOperation;
  }
#endif
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  // Check the return value: GraphIdPtr should be valid.
  uint32_t *GraphId = MemInst->getPointer<uint32_t *>(GraphIdPtr);
  if (unlikely(GraphId == nullptr)) {
    spdlog::error(
        "[WASI-NN] Failed when accessing the return GraphID memory."sv);
    return WASINN::ErrNo::InvalidArgument;
  }
  // Get and check the device.
  const auto Device = static_cast<WASINN::Device>(Target);
  switch (Device) {
  case WASINN::Device::CPU:
  case WASINN::Device::GPU:
  case WASINN::Device::TPU:
    break;
  default:
    spdlog::error("[WASI-NN] Unknown device {}."sv, Target);
    return WASINN::ErrNo::InvalidArgument;
  }
  spdlog::debug("[WASI-NN] Using device: {}."sv, Device);

  // Builders' Layout:
  //   | builder-0 | builder-0 len | builder-1 | builder-1 len | ...
  struct WasiBuilderPair {
    uint32_t Ptr;
    uint32_t Len;
  };

  const auto WasiBuilders =
      MemInst->getSpan<const WasiBuilderPair>(BuilderPtr, BuilderLen);
  if (unlikely(WasiBuilders.size() != BuilderLen)) {
    spdlog::error("[WASI-NN] Failed when accessing the GraphBuilder memory."sv);
    return WASINN::ErrNo::InvalidArgument;
  }

  std::vector<Span<uint8_t>> Builders;
  Builders.reserve(BuilderLen);
  for (size_t I = 0; I < WasiBuilders.size(); ++I) {
    const auto &WasiBuilder = WasiBuilders[I];
    auto Builder = MemInst->getSpan<uint8_t>(EndianValue(WasiBuilder.Ptr).le(),
                                             EndianValue(WasiBuilder.Len).le());
    if (unlikely(Builder.size() != EndianValue(WasiBuilder.Len).le())) {
      spdlog::error("[WASI-NN] Failed when accessing the Builder[{}] memory."sv,
                    I);
      return WASINN::ErrNo::InvalidArgument;
    }
    Builders.emplace_back(Builder);
  }
  auto Backend = static_cast<WASINN::Backend>(RawEncoding);
  uint32_t NewGraphId = 0;
  auto Res = load(Env, Builders, Backend, Device, ""sv, NewGraphId);
  if (Res.has_value() && *Res == WASINN::ErrNo::Success) {
    *GraphId = EndianValue(NewGraphId).le();
  }
  return Res;
}

Expect<WASINN::ErrNo>
WasiNNLoadByName::bodyImpl(const Runtime::CallingFrame &Frame, uint32_t NamePtr,
                           uint32_t NameLen, uint32_t GraphIdPtr) {
  Env.setEnviron(&Frame);
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  // Check the return value: GraphIdPtr should be valid.
  uint32_t *GraphId = MemInst->getPointer<uint32_t *>(GraphIdPtr);
  if (unlikely(GraphId == nullptr)) {
    spdlog::error(
        "[WASI-NN] Failed when accessing the return GraphID memory."sv);
    return WASINN::ErrNo::InvalidArgument;
  }

  // Get the model name.
  if (unlikely(!MemInst->checkAccessBound(NamePtr, NameLen))) {
    spdlog::error("[WASI-NN] Failed when accessing the Name memory."sv);
    return WASINN::ErrNo::InvalidArgument;
  }
  auto NameStrView = MemInst->getStringView(NamePtr, NameLen);

#ifdef WASMEDGE_BUILD_WASI_NN_RPC
  if (Env.NNRPCChannel != nullptr) {
    auto Stub = wasi_ephemeral_nn::Graph::NewStub(Env.NNRPCChannel);
    grpc::ClientContext ClientContext;
    wasi_ephemeral_nn::LoadByNameRequest Req;
    Req.set_name(NameStrView.data(), NameStrView.size());
    wasi_ephemeral_nn::LoadByNameResult Res;
    auto Status = Stub->LoadByName(&ClientContext, Req, &Res);
    if (!Status.ok()) {
      auto Metadata = ClientContext.GetServerTrailingMetadata();
      return metadataToErrNo(Metadata);
    }
    *GraphId = Res.graph_handle();
    return WASINN::ErrNo::Success;
  }
#endif // ifdef WASMEDGE_BUILD_WASI_NN_RPC

  // Get the model.
  std::string ModelName(NameStrView);
  uint32_t FoundGraphId = 0;
  if (Env.mdGet(ModelName, FoundGraphId)) {
    *GraphId = EndianValue(FoundGraphId).le();
    return WASINN::ErrNo::Success;
  }
  auto Res = Env.mdBuild(ModelName, FoundGraphId, load);
  if (Res.has_value() && *Res == WASINN::ErrNo::Success) {
    *GraphId = EndianValue(FoundGraphId).le();
  }
  return Res;
}

Expect<WASINN::ErrNo> WasiNNLoadByNameWithConfig::bodyImpl(
    const Runtime::CallingFrame &Frame, uint32_t NamePtr, uint32_t NameLen,
    uint32_t ConfigPtr, uint32_t ConfigLen, uint32_t GraphIdPtr) {
  Env.setEnviron(&Frame);
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  // Check the return value: GraphIdPtr should be valid.
  auto GraphId = MemInst->getPointer<uint32_t *>(GraphIdPtr);
  if (unlikely(GraphId == nullptr)) {
    spdlog::error(
        "[WASI-NN] Failed when accessing the return GraphID memory."sv);
    return WASINN::ErrNo::InvalidArgument;
  }

  // Get the model name.
  if (unlikely(!MemInst->checkAccessBound(NamePtr, NameLen))) {
    spdlog::error("[WASI-NN] Failed when accessing the Name memory."sv);
    return WASINN::ErrNo::InvalidArgument;
  }
  auto NameStrView = MemInst->getStringView(NamePtr, NameLen);

  // Get the model config.
  if (unlikely(!MemInst->checkAccessBound(ConfigPtr, ConfigLen))) {
    spdlog::error("[WASI-NN] Failed when accessing the Config memory."sv);
    return WASINN::ErrNo::InvalidArgument;
  }
  auto ConfigSpan = MemInst->getSpan<const uint8_t>(ConfigPtr, ConfigLen);

#ifdef WASMEDGE_BUILD_WASI_NN_RPC
  if (Env.NNRPCChannel != nullptr) {
    auto Stub = wasi_ephemeral_nn::Graph::NewStub(Env.NNRPCChannel);
    grpc::ClientContext ClientContext;
    wasi_ephemeral_nn::LoadByNameWithConfigRequest Req;
    Req.set_name(NameStrView.data(), NameStrView.size());
    Req.set_config(reinterpret_cast<const char *>(ConfigSpan.data()),
                   ConfigSpan.size());
    wasi_ephemeral_nn::LoadByNameWithConfigResult Res;
    auto Status = Stub->LoadByNameWithConfig(&ClientContext, Req, &Res);
    if (!Status.ok()) {
      auto Metadata = ClientContext.GetServerTrailingMetadata();
      return metadataToErrNo(Metadata);
    }
    *GraphId = Res.graph_handle();
    return WASINN::ErrNo::Success;
  }
#endif // ifdef WASMEDGE_BUILD_WASI_NN_RPC

  // Get the model.
  std::string ModelName(NameStrView);
  std::vector<uint8_t> ModelConfig(ConfigSpan.begin(), ConfigSpan.end());
  uint32_t FoundGraphId = 0;
  if (Env.mdGet(ModelName, FoundGraphId)) {
    *GraphId = EndianValue(FoundGraphId).le();
    return WASINN::ErrNo::Success;
  }
  auto Res = Env.mdBuild(ModelName, FoundGraphId, load, ModelConfig);
  if (Res.has_value() && *Res == WASINN::ErrNo::Success) {
    *GraphId = EndianValue(FoundGraphId).le();
  }
  return Res;
}

Expect<WASINN::ErrNo>
WasiNNInitExecCtx::bodyImpl(const Runtime::CallingFrame &Frame,
                            uint32_t GraphId, uint32_t ContextPtr) {
  Env.setEnviron(&Frame);
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  // Check the return value: Context should be valid.
  uint32_t *Context = MemInst->getPointer<uint32_t *>(ContextPtr);
  if (unlikely(Context == nullptr)) {
    spdlog::error("[WASI-NN] Failed when accessing the Context memory."sv);
    return WASINN::ErrNo::InvalidArgument;
  }

#ifdef WASMEDGE_BUILD_WASI_NN_RPC
  if (Env.NNRPCChannel != nullptr) {
    auto Stub = wasi_ephemeral_nn::GraphResource::NewStub(Env.NNRPCChannel);
    grpc::ClientContext ClientContext;
    wasi_ephemeral_nn::InitExecutionContextRequest Req;
    Req.set_resource_handle(GraphId);
    wasi_ephemeral_nn::InitExecutionContextResult Res;
    auto Status = Stub->InitExecutionContext(&ClientContext, Req, &Res);
    if (!Status.ok()) {
      auto Metadata = ClientContext.GetServerTrailingMetadata();
      return metadataToErrNo(Metadata);
    }
    *Context = Res.ctx_handle();
    return WASINN::ErrNo::Success;
  }
#endif // ifdef WASMEDGE_BUILD_WASI_NN_RPC

  return Env.withGraphOp(
      GraphId, WASINN::HostOp::InitExecCtx,
      [&](const std::shared_ptr<WASINN::Graph> &G) -> Expect<WASINN::ErrNo> {
        auto NewContext = std::make_shared<WASINN::Context>(GraphId, G);
        Expect<WASINN::ErrNo> Res = WASINN::ErrNo::Success;
        switch (const auto Backend = G->getBackend()) {
#define EACH(B)                                                                \
  case WASINN::Backend::B:                                                     \
    Res = WASINN::B::initExecCtx(Env, *G, *NewContext);                        \
    break;
          FOR_EACH_BACKEND(EACH)
#undef EACH
        default:
          reportUnknownBackend(Backend);
          return WASINN::ErrNo::InvalidEncoding;
        }
        if (!Res.has_value() || *Res != WASINN::ErrNo::Success) {
          return Res;
        }
        const auto Id = Env.NNContext.insert(std::move(NewContext));
        if (!Id.has_value()) {
          spdlog::error("[WASI-NN] init_execution_context: context handle "
                        "space is exhausted."sv);
          return WASINN::ErrNo::RuntimeError;
        }
        *Context = EndianValue(*Id).le();
        return WASINN::ErrNo::Success;
      });
}

Expect<WASINN::ErrNo>
WasiNNSetInput::bodyImpl(const Runtime::CallingFrame &Frame, uint32_t ContextId,
                         uint32_t Index, uint32_t TensorPtr) {
  Env.setEnviron(&Frame);
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  // Tensor's Layout:
  //   | dim buf | dim buf len | rtype | data buf | data buf len |
  struct WasiTensorData {
    uint32_t DimensionPtr;
    uint32_t DimensionLen;
    uint32_t RType;
    uint32_t TensorPtr;
    uint32_t TensorLen;
  };
  // Get the tensor.
  auto *WasiTensor = MemInst->getPointer<const WasiTensorData *>(TensorPtr);
  if (unlikely(WasiTensor == nullptr)) {
    spdlog::error("[WASI-NN] Failed when accessing the Tensor memory."sv);
    return WASINN::ErrNo::InvalidArgument;
  }

  WASINN::TensorData Tensor;
  Tensor.Dimension =
      MemInst->getSpan<uint32_t>(EndianValue(WasiTensor->DimensionPtr).le(),
                                 EndianValue(WasiTensor->DimensionLen).le());
  if (unlikely(Tensor.Dimension.size() !=
               EndianValue(WasiTensor->DimensionLen).le())) {
    spdlog::error("[WASI-NN] Failed when accessing the Dimension memory."sv);
    return WASINN::ErrNo::InvalidArgument;
  }
  Tensor.Tensor =
      MemInst->getSpan<uint8_t>(EndianValue(WasiTensor->TensorPtr).le(),
                                EndianValue(WasiTensor->TensorLen).le());
  if (unlikely(Tensor.Tensor.size() !=
               EndianValue(WasiTensor->TensorLen).le())) {
    spdlog::error("[WASI-NN] Failed when accessing the TensorData memory."sv);
    return WASINN::ErrNo::InvalidArgument;
  }
  switch (const auto RType = static_cast<WASINN::TensorType>(
              EndianValue(WasiTensor->RType).le())) {
  case WASINN::TensorType::F16:
  case WASINN::TensorType::F32:
  case WASINN::TensorType::U8:
  case WASINN::TensorType::I32:
    Tensor.RType = RType;
    break;
  default:
    spdlog::error("[WASI-NN] Unknown tensor type {}."sv,
                  static_cast<uint32_t>(RType));
    return WASINN::ErrNo::InvalidArgument;
  }

#ifdef WASMEDGE_BUILD_WASI_NN_RPC
  if (Env.NNRPCChannel != nullptr) {
    auto Stub = wasi_ephemeral_nn::GraphExecutionContextResource::NewStub(
        Env.NNRPCChannel);
    grpc::ClientContext ClientContext;
    wasi_ephemeral_nn::SetInputRequest Req;
    Req.set_resource_handle(ContextId);
    Req.set_index(Index);
    wasi_ephemeral_nn::Tensor RPCTensor;
    RPCTensor.mutable_dimensions()->Add(Tensor.Dimension.begin(),
                                        Tensor.Dimension.end());
    RPCTensor.set_ty(wasi_ephemeral_nn::TensorType(Tensor.RType));
    RPCTensor.set_data(reinterpret_cast<const char *>(Tensor.Tensor.data()),
                       Tensor.Tensor.size());
    *Req.mutable_tensor() = RPCTensor;
    google::protobuf::Empty Res;
    auto Status = Stub->SetInput(&ClientContext, Req, &Res);
    if (!Status.ok()) {
      auto Metadata = ClientContext.GetServerTrailingMetadata();
      return metadataToErrNo(Metadata);
    }
    return WASINN::ErrNo::Success;
  }
#endif // ifdef WASMEDGE_BUILD_WASI_NN_RPC

  return Env.withContextOp(
      ContextId, WASINN::HostOp::SetInput,
      [&](WASINN::Graph &G, WASINN::Context &C) -> Expect<WASINN::ErrNo> {
        switch (const auto Backend = C.getBackend()) {
#define EACH(B)                                                                \
  case WASINN::Backend::B:                                                     \
    return WASINN::B::setInput(Env, G, C, Index, Tensor);
          FOR_EACH_BACKEND(EACH)
#undef EACH
        default:
          reportUnknownBackend(Backend);
          return WASINN::ErrNo::InvalidEncoding;
        }
      });
}

Expect<WASINN::ErrNo>
WasiNNGetOutput::bodyImpl(const Runtime::CallingFrame &Frame,
                          uint32_t ContextId, uint32_t Index,
                          uint32_t OutBufferPtr, uint32_t OutBufferMaxSize,
                          uint32_t BytesWrittenPtr) {
  Env.setEnviron(&Frame);
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const auto OutBuffer =
      MemInst->getSpan<uint8_t>(OutBufferPtr, OutBufferMaxSize);
  if (unlikely(OutBuffer.data() == nullptr)) {
    spdlog::error(
        "[WASI-NN] Failed when accessing the Output Buffer memory."sv);
    return WASINN::ErrNo::InvalidArgument;
  }
  uint32_t *BytesWritten = MemInst->getPointer<uint32_t *>(BytesWrittenPtr);
  if (unlikely(BytesWritten == nullptr)) {
    spdlog::error("[WASI-NN] Failed when accessing the BytesWritten memory."sv);
    return WASINN::ErrNo::InvalidArgument;
  }

#ifdef WASMEDGE_BUILD_WASI_NN_RPC
  if (Env.NNRPCChannel != nullptr) {
    auto Stub = wasi_ephemeral_nn::GraphExecutionContextResource::NewStub(
        Env.NNRPCChannel);
    grpc::ClientContext ClientContext;
    wasi_ephemeral_nn::GetOutputRequest Req;
    Req.set_resource_handle(ContextId);
    Req.set_index(Index);
    wasi_ephemeral_nn::GetOutputResult Res;
    auto Status = Stub->GetOutput(&ClientContext, Req, &Res);
    if (!Status.ok()) {
      *BytesWritten = 0;
      auto Metadata = ClientContext.GetServerTrailingMetadata();
      return metadataToErrNo(Metadata);
    }
    *BytesWritten = static_cast<uint32_t>(Res.data().size());
    if (OutBufferMaxSize < Res.data().size()) {
      spdlog::error("[WASI-NN] get_output: output buffer too small, "
                    "need {} bytes but got {}."sv,
                    Res.data().size(), OutBufferMaxSize);
      return WASINN::ErrNo::TooLarge;
    }
    std::copy_n(Res.data().begin(), Res.data().size(), OutBuffer.begin());
    return WASINN::ErrNo::Success;
  }
#endif // ifdef WASMEDGE_BUILD_WASI_NN_RPC

  return Env.withContextOp(
      ContextId, WASINN::HostOp::GetOutput,
      [&](WASINN::Graph &G, WASINN::Context &C) -> Expect<WASINN::ErrNo> {
        switch (const auto Backend = C.getBackend()) {
#define EACH(B)                                                                \
  case WASINN::Backend::B:                                                     \
    return WASINN::B::getOutput(Env, G, C, Index, OutBuffer, *BytesWritten);
          FOR_EACH_BACKEND(EACH)
#undef EACH
        default:
          reportUnknownBackend(Backend);
          return WASINN::ErrNo::InvalidEncoding;
        }
      });
}

Expect<WASINN::ErrNo> WasiNNGetOutputSingle::bodyImpl(
    const Runtime::CallingFrame &Frame, uint32_t ContextId, uint32_t Index,
    uint32_t OutBufferPtr, uint32_t OutBufferMaxSize,
    uint32_t BytesWrittenPtr) {
  Env.setEnviron(&Frame);
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const auto OutBuffer =
      MemInst->getSpan<uint8_t>(OutBufferPtr, OutBufferMaxSize);
  if (unlikely(OutBuffer.data() == nullptr)) {
    spdlog::error(
        "[WASI-NN] Failed when accessing the Output Buffer memory."sv);
    return WASINN::ErrNo::InvalidArgument;
  }
  uint32_t *BytesWritten = MemInst->getPointer<uint32_t *>(BytesWrittenPtr);
  if (unlikely(BytesWritten == nullptr)) {
    spdlog::error("[WASI-NN] Failed when accessing the BytesWritten memory."sv);
    return WASINN::ErrNo::InvalidArgument;
  }

#ifdef WASMEDGE_BUILD_WASI_NN_RPC
  if (Env.NNRPCChannel != nullptr) {
    auto Stub = wasi_ephemeral_nn::GraphExecutionContextResource::NewStub(
        Env.NNRPCChannel);
    grpc::ClientContext ClientContext;
    wasi_ephemeral_nn::GetOutputRequest Req;
    Req.set_resource_handle(ContextId);
    Req.set_index(Index);
    wasi_ephemeral_nn::GetOutputResult Res;
    auto Status = Stub->GetOutputSingle(&ClientContext, Req, &Res);
    if (!Status.ok()) {
      *BytesWritten = 0;
      auto Metadata = ClientContext.GetServerTrailingMetadata();
      return metadataToErrNo(Metadata);
    }
    *BytesWritten = static_cast<uint32_t>(Res.data().size());
    if (OutBufferMaxSize < Res.data().size()) {
      spdlog::error("[WASI-NN] get_output_single: output buffer too small, "
                    "need {} bytes but got {}."sv,
                    Res.data().size(), OutBufferMaxSize);
      return WASINN::ErrNo::TooLarge;
    }
    std::copy_n(Res.data().begin(), Res.data().size(), OutBuffer.begin());
    return WASINN::ErrNo::Success;
  }
#endif // ifdef WASMEDGE_BUILD_WASI_NN_RPC

  return Env.withContextOp(
      ContextId, WASINN::HostOp::GetOutputSingle,
      [&](WASINN::Graph &G, WASINN::Context &C) -> Expect<WASINN::ErrNo> {
        switch (C.getBackend()) {
        case WASINN::Backend::GGML:
          return WASINN::GGML::getOutputSingle(Env, G, C, Index, OutBuffer,
                                               *BytesWritten);
        case WASINN::Backend::BitNet:
          return WASINN::BitNet::getOutputSingle(Env, G, C, Index, OutBuffer,
                                                 *BytesWritten);
        default:
          spdlog::error("[WASI-NN] get_output_single: Only GGML and BitNet "
                        "backend supports get_output_single."sv);
          return WASINN::ErrNo::InvalidArgument;
        }
      });
}

Expect<WASINN::ErrNo>
WasiNNCompute::bodyImpl(const Runtime::CallingFrame &Frame,
                        uint32_t ContextId) {
  Env.setEnviron(&Frame);
#ifdef WASMEDGE_BUILD_WASI_NN_RPC
  if (Env.NNRPCChannel != nullptr) {
    auto Stub = wasi_ephemeral_nn::GraphExecutionContextResource::NewStub(
        Env.NNRPCChannel);
    grpc::ClientContext ClientContext;
    wasi_ephemeral_nn::ComputeRequest Req;
    Req.set_resource_handle(ContextId);
    google::protobuf::Empty Res;
    auto Status = Stub->Compute(&ClientContext, Req, &Res);
    if (!Status.ok()) {
      auto Metadata = ClientContext.GetServerTrailingMetadata();
      return metadataToErrNo(Metadata);
    }
    return WASINN::ErrNo::Success;
  }
#endif // ifdef WASMEDGE_BUILD_WASI_NN_RPC
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return Env.withContextOp(
      ContextId, WASINN::HostOp::Compute,
      [&](WASINN::Graph &G, WASINN::Context &C) -> Expect<WASINN::ErrNo> {
        switch (const auto Backend = C.getBackend()) {
#define EACH(B)                                                                \
  case WASINN::Backend::B:                                                     \
    return WASINN::B::compute(Env, G, C);
          FOR_EACH_BACKEND(EACH)
#undef EACH
        default:
          reportUnknownBackend(Backend);
          return WASINN::ErrNo::InvalidEncoding;
        }
      });
}

Expect<WASINN::ErrNo>
WasiNNComputeSingle::bodyImpl(const Runtime::CallingFrame &Frame,
                              uint32_t ContextId) {
  Env.setEnviron(&Frame);
#ifdef WASMEDGE_BUILD_WASI_NN_RPC
  if (Env.NNRPCChannel != nullptr) {
    auto Stub = wasi_ephemeral_nn::GraphExecutionContextResource::NewStub(
        Env.NNRPCChannel);
    grpc::ClientContext ClientContext;
    wasi_ephemeral_nn::ComputeRequest Req;
    Req.set_resource_handle(ContextId);
    google::protobuf::Empty Res;
    auto Status = Stub->ComputeSingle(&ClientContext, Req, &Res);
    if (!Status.ok()) {
      auto Metadata = ClientContext.GetServerTrailingMetadata();
      return metadataToErrNo(Metadata);
    }
    return WASINN::ErrNo::Success;
  }
#endif // ifdef WASMEDGE_BUILD_WASI_NN_RPC
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return Env.withContextOp(
      ContextId, WASINN::HostOp::ComputeSingle,
      [&](WASINN::Graph &G, WASINN::Context &C) -> Expect<WASINN::ErrNo> {
        switch (C.getBackend()) {
        case WASINN::Backend::GGML:
          return WASINN::GGML::computeSingle(Env, G, C);
        case WASINN::Backend::BitNet:
          return WASINN::BitNet::computeSingle(Env, G, C);
        default:
          spdlog::error("[WASI-NN] compute_single: Only GGML and BitNet "
                        "backend supports compute_single."sv);
          return WASINN::ErrNo::InvalidArgument;
        }
      });
}

Expect<WASINN::ErrNo>
WasiNNFiniSingle::bodyImpl(const Runtime::CallingFrame &Frame,
                           uint32_t ContextId) {
  Env.setEnviron(&Frame);
#ifdef WASMEDGE_BUILD_WASI_NN_RPC
  if (Env.NNRPCChannel != nullptr) {
    auto Stub = wasi_ephemeral_nn::GraphExecutionContextResource::NewStub(
        Env.NNRPCChannel);
    grpc::ClientContext ClientContext;
    wasi_ephemeral_nn::FiniSingleRequest Req;
    Req.set_resource_handle(ContextId);
    google::protobuf::Empty Res;
    auto Status = Stub->FiniSingle(&ClientContext, Req, &Res);
    if (!Status.ok()) {
      auto Metadata = ClientContext.GetServerTrailingMetadata();
      return metadataToErrNo(Metadata);
    }
    return WASINN::ErrNo::Success;
  }
#endif // ifdef WASMEDGE_BUILD_WASI_NN_RPC
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return Env.withContextOp(
      ContextId, WASINN::HostOp::FiniSingle,
      [&](WASINN::Graph &G, WASINN::Context &C) -> Expect<WASINN::ErrNo> {
        switch (C.getBackend()) {
        case WASINN::Backend::GGML:
          return WASINN::GGML::finiSingle(Env, G, C);
        case WASINN::Backend::BitNet:
          return WASINN::BitNet::finiSingle(Env, G, C);
        default:
          spdlog::error("[WASI-NN] fini_single: Only GGML and BitNet backend "
                        "supports fini_single."sv);
          return WASINN::ErrNo::InvalidArgument;
        }
      });
}

Expect<WASINN::ErrNo> WasiNNUnload::bodyImpl(const Runtime::CallingFrame &Frame,
                                             uint32_t GraphId) {
  Env.setEnviron(&Frame);
#ifdef WASMEDGE_BUILD_WASI_NN_RPC
  if (Env.NNRPCChannel != nullptr) {
    // TODO: implement RPC for unload
    spdlog::error("[WASI-NN] RPC client is not implemented for unload"sv);
    return WASINN::ErrNo::UnsupportedOperation;
  }
#endif
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return Env.unloadGraph(GraphId);
}

Expect<WASINN::ErrNo>
WasiNNFinalizeExecCtx::bodyImpl(const Runtime::CallingFrame &Frame,
                                uint32_t ContextId) {
  Env.setEnviron(&Frame);
#ifdef WASMEDGE_BUILD_WASI_NN_RPC
  if (Env.NNRPCChannel != nullptr) {
    // TODO: implement RPC for finalize_execution_context
    spdlog::error("[WASI-NN] RPC client is not implemented for "sv
                  "finalize_execution_context"sv);
    return WASINN::ErrNo::UnsupportedOperation;
  }
#endif
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return Env.finalizeContext(ContextId);
}

} // namespace Host
} // namespace WasmEdge
