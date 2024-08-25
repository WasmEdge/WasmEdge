// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "tfl.h"
#include "wasinnenv.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TFLITE
#include "tensorflow/lite/c/common.h"
#endif

namespace WasmEdge::Host::WASINN::TensorflowLite {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TFLITE
Expect<WASINN::ErrNo> load(WASINN::WasiNNEnvironment &Env,
                           Span<const Span<uint8_t>> Builders,
                           WASINN::Device Device, uint32_t &GraphId) noexcept {
  if ((Device != WASINN::Device::CPU)) {
    spdlog::error("[WASI-NN] TensorflowLite Only support CPU target.");
    return WASINN::ErrNo::InvalidArgument;
  }
  // The graph builder length must be 1.
  if (Builders.size() != 1) {
    spdlog::error("[WASI-NN] Wrong GraphBuilder Length {:d}, expect 1",
                  Builders.size());
    return WASINN::ErrNo::InvalidArgument;
  }
  // Add a new graph.
  Env.NNGraph.emplace_back(WASINN::Backend::TensorflowLite);
  auto &GraphRef = Env.NNGraph.back().get<Graph>();

  // Copy graph builder data to TfLiteModData and create a new TfLiteModel.
  GraphRef.TfLiteModData.assign(Builders[0].begin(), Builders[0].end());
  GraphRef.TFLiteMod = TfLiteModelCreate(GraphRef.TfLiteModData.data(),
                                         GraphRef.TfLiteModData.size());
  if (unlikely(GraphRef.TFLiteMod == nullptr)) {
    spdlog::error("[WASI-NN] Cannot import TFLite model");
    Env.NNGraph.pop_back();
    return WASINN::ErrNo::InvalidArgument;
  }

  // Store the loaded graph.
  GraphId = Env.NNGraph.size() - 1;
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> initExecCtx(WASINN::WasiNNEnvironment &Env,
                                  uint32_t GraphId,
                                  uint32_t &ContextId) noexcept {
  // Check the network and the execution network with the graph ID.
  if (Env.NNGraph[GraphId].get<Graph>().TFLiteMod == nullptr) {
    spdlog::error("[WASI-NN] Model for Graph:{} is missing!", GraphId);
    return WASINN::ErrNo::MissingMemory;
  }

  // Create context.
  Env.NNContext.emplace_back(GraphId, Env.NNGraph[GraphId]);
  auto &CxtRef = Env.NNContext.back().get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  auto *TFLiteOps = TfLiteInterpreterOptionsCreate();
  TfLiteInterpreterOptionsSetNumThreads(TFLiteOps, 2);
  CxtRef.TFLiteInterp = TfLiteInterpreterCreate(GraphRef.TFLiteMod, TFLiteOps);
  TfLiteInterpreterOptionsDelete(TFLiteOps);
  if (unlikely(CxtRef.TFLiteInterp == nullptr)) {
    spdlog::error("[WASI-NN] Cannot create TFLite interpreter.");
    Env.NNContext.pop_back();
    return WASINN::ErrNo::Busy;
  }
  TfLiteInterpreterAllocateTensors(CxtRef.TFLiteInterp);

  ContextId = Env.NNContext.size() - 1;
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> setInput(WASINN::WasiNNEnvironment &Env,
                               uint32_t ContextId, uint32_t Index,
                               const WASINN::TensorData &Tensor) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  uint32_t InCnt = TfLiteInterpreterGetInputTensorCount(CxtRef.TFLiteInterp);
  if (Index >= InCnt) {
    spdlog::error("[WASI-NN] Invalid index id {} for the input, only {} "
                  "inputs are allowed",
                  Index, InCnt);
    return WASINN::ErrNo::InvalidArgument;
  }

  auto *HoldTensor =
      TfLiteInterpreterGetInputTensor(CxtRef.TFLiteInterp, Index);
  // Check the input data size.
  const auto HoldTensorByteSize = TfLiteTensorByteSize(HoldTensor);
  if (HoldTensorByteSize != Tensor.Tensor.size()) {
    spdlog::error("[WASI-NN] Expect tensor byte size {}, but got {}",
                  HoldTensorByteSize, Tensor.Tensor.size());
    return WASINN::ErrNo::InvalidArgument;
  }
  // Check the input tensor dimensions.
  const auto HoldTensorNumDims = TfLiteTensorNumDims(HoldTensor);
  if (static_cast<size_t>(HoldTensorNumDims) != Tensor.Dimension.size()) {
    spdlog::error("[WASI-NN] Expect tensor number of dimensions {}, but got {}",
                  HoldTensorNumDims, Tensor.Dimension.size());
    return WASINN::ErrNo::InvalidArgument;
  }
  for (uint32_t I = 0; I < Tensor.Dimension.size(); I++) {
    const auto HoldTensorDim = TfLiteTensorDim(HoldTensor, I);
    if (static_cast<uint32_t>(HoldTensorDim) != Tensor.Dimension[I]) {
      spdlog::error("[WASI-NN] Expect tensor dimension[{}] = {}, but got {}", I,
                    HoldTensorDim, Tensor.Dimension[I]);
      return WASINN::ErrNo::InvalidArgument;
    }
  }
  // Check the input tensor type.
  WASINN::TensorType LiteType;
  switch (const auto Type = TfLiteTensorType(HoldTensor)) {
  case TfLiteType::kTfLiteUInt8:
    LiteType = WASINN::TensorType::U8;
    break;
  case TfLiteType::kTfLiteFloat16:
    LiteType = WASINN::TensorType::F16;
    break;
  case TfLiteType::kTfLiteFloat32:
    LiteType = WASINN::TensorType::F32;
    break;
  case TfLiteType::kTfLiteInt32:
    LiteType = WASINN::TensorType::I32;
    break;
  default:
    spdlog::error("[WASI-NN] Unsupported TFLite type: {}",
                  TfLiteTypeGetName(Type));
    return WASINN::ErrNo::InvalidArgument;
  }

  if (unlikely(LiteType != Tensor.RType)) {
    spdlog::error("[WASI-NN] Expect tensor type {}, but got {}", LiteType,
                  Tensor.RType);
    return WASINN::ErrNo::InvalidArgument;
  }
  TfLiteStatus Stat = TfLiteTensorCopyFromBuffer(
      HoldTensor, Tensor.Tensor.data(), Tensor.Tensor.size());
  if (unlikely(Stat != TfLiteStatus::kTfLiteOk)) {
    spdlog::error("[WASI-NN] Copy tensor memory failed");
    return WASINN::ErrNo::Busy;
  }

  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> getOutput(WASINN::WasiNNEnvironment &Env,
                                uint32_t ContextId, uint32_t Index,
                                Span<uint8_t> OutBuffer,
                                uint32_t &BytesWritten) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  uint32_t OutCnt = TfLiteInterpreterGetOutputTensorCount(CxtRef.TFLiteInterp);
  if (Index >= OutCnt) {
    spdlog::error("[WASI-NN] Invalid index id {} for the input, only {} "
                  "outputs are allowed",
                  Index, OutCnt);
    return WASINN::ErrNo::InvalidArgument;
  }
  const TfLiteTensor *HoldTensor =
      TfLiteInterpreterGetOutputTensor(CxtRef.TFLiteInterp, Index);
  const uint32_t BytesToWrite = TfLiteTensorByteSize(HoldTensor);
  // Check out buffer max size.
  if (OutBuffer.size() < BytesToWrite) {
    spdlog::error("[WASI-NN] Expect out buffer max size {}, but got {}",
                  BytesToWrite, OutBuffer.size());
    return WASINN::ErrNo::InvalidArgument;
  }
  TfLiteTensorCopyToBuffer(HoldTensor, OutBuffer.data(), BytesToWrite);
  BytesWritten = BytesToWrite;
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> compute(WASINN::WasiNNEnvironment &Env,
                              uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  // Run session
  if (unlikely(CxtRef.TFLiteInterp == nullptr)) {
    spdlog::error("[WASI-NN] Tensorflow Lite context empty");
    return WASINN::ErrNo::MissingMemory;
  }
  TfLiteStatus Stat = TfLiteInterpreterInvoke(CxtRef.TFLiteInterp);
  if (unlikely(Stat != TfLiteStatus::kTfLiteOk)) {
    spdlog::error("[WASI-NN] Invocation failed.");
    return WASINN::ErrNo::Busy;
  }
  return WASINN::ErrNo::Success;
}
#else
namespace {
Expect<WASINN::ErrNo> reportBackendNotSupported() noexcept {
  spdlog::error(
      "[WASI-NN] TensorflowLite backend is not built. use "
      "-WASMEDGE_PLUGIN_WASI_NN_BACKEND=\"Tensorflowlite\" to build it.");
  return WASINN::ErrNo::InvalidArgument;
}
} // namespace

Expect<WASINN::ErrNo> load(WASINN::WasiNNEnvironment &,
                           Span<const Span<uint8_t>>, WASINN::Device,
                           uint32_t &) noexcept {
  return reportBackendNotSupported();
}
Expect<WASINN::ErrNo> initExecCtx(WASINN::WasiNNEnvironment &, uint32_t,
                                  uint32_t &) noexcept {
  return reportBackendNotSupported();
}
Expect<WASINN::ErrNo> setInput(WASINN::WasiNNEnvironment &, uint32_t, uint32_t,
                               const TensorData &) noexcept {
  return reportBackendNotSupported();
}
Expect<WASINN::ErrNo> getOutput(WASINN::WasiNNEnvironment &, uint32_t, uint32_t,
                                Span<uint8_t>, uint32_t &) noexcept {
  return reportBackendNotSupported();
}
Expect<WASINN::ErrNo> compute(WASINN::WasiNNEnvironment &, uint32_t) noexcept {
  return reportBackendNotSupported();
}

#endif
} // namespace WasmEdge::Host::WASINN::TensorflowLite
