// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "torch.h"
#include "wasinnenv.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TORCH
#include <torch/torch.h>
#endif

namespace WasmEdge::Host::WASINN::PyTorch {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TORCH
Expect<ErrNo> load(WasiNNEnvironment &Env, Span<const Span<uint8_t>> Builders,
                   Device Device, uint32_t &GraphId) noexcept {
  // The graph builder length must be 1.
  if (Builders.size() != 1) {
    spdlog::error("[WASI-NN] Wrong GraphBuilder Length {:d}, expect 1",
                  Builders.size());
    return ErrNo::InvalidArgument;
  }

  auto Weight = Builders[0];
  // Add a new graph.
  Env.NNGraph.emplace_back(Backend::PyTorch);
  auto &GraphRef = Env.NNGraph.back().get<Graph>();
  // Setup Graph Device
  if (Device == Device::CPU) {
    GraphRef.TorchDevice = at::kCPU;
  } else if (Device == Device::GPU) {
    if (!torch::cuda::is_available()) {
      spdlog::error(
          "[WASI-NN] CUDA Unavailable, platform Cannot support GPU target.");
      return ErrNo::InvalidArgument;
    }
    GraphRef.TorchDevice = at::kCUDA;
  } else {
    spdlog::error("[WASI-NN] PyTorch Only support CPU and GPU target.");
    return ErrNo::InvalidArgument;
  }

  std::istringstream BinRead(
      std::string(reinterpret_cast<char *>(Weight.data()), Weight.size()));

  try {
    GraphRef.TorchModel = torch::jit::load(BinRead);
    GraphRef.TorchModel.to(GraphRef.TorchDevice);
  } catch (const c10::Error &e) {
    spdlog::error("[WASI-NN] Failed when load the TorchScript model.");
    Env.NNGraph.pop_back();
    return ErrNo::InvalidArgument;
  }
  // Store the loaded graph.
  GraphId = Env.NNGraph.size() - 1;
  return ErrNo::Success;
}

Expect<ErrNo> initExecCtx(WasiNNEnvironment &Env, uint32_t GraphId,
                          uint32_t &ContextId) noexcept {
  Env.NNContext.emplace_back(GraphId, Env.NNGraph[GraphId]);

  ContextId = Env.NNContext.size() - 1;
  return ErrNo::Success;
}

Expect<ErrNo> setInput(WasiNNEnvironment &Env, uint32_t ContextId,
                       uint32_t Index, const TensorData &Tensor) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  if (Index >= CxtRef.TorchInputs.size()) {
    CxtRef.TorchInputs.resize(Index + 1);
  }
  if (Tensor.RType != TensorType::F32) {
    spdlog::error(
        "[WASI-NN] Only F32 inputs and outputs are supported for now.");
    return ErrNo::InvalidArgument;
  }
  auto Options =
      torch::TensorOptions().dtype(torch::kFloat32).requires_grad(false);
  std::vector<int64_t> Dims;
  for (size_t I = 0; I < Tensor.Dimension.size(); I++) {
    Dims.push_back(static_cast<int64_t>(Tensor.Dimension[I]));
  }
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  torch::Tensor InTensor =
      torch::from_blob(reinterpret_cast<float *>(Tensor.Tensor.data()), Dims,
                       Options)
          .to(GraphRef.TorchDevice);

  CxtRef.TorchInputs[Index] = InTensor.clone();
  return ErrNo::Success;
}

Expect<ErrNo> getOutput(WasiNNEnvironment &Env, uint32_t ContextId,
                        uint32_t Index, Span<uint8_t> OutBuffer,
                        uint32_t &BytesWritten) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  if (CxtRef.TorchOutputs.size() <= Index) {
    spdlog::error(
        "[WASI-NN] The output index {} exceeds the outputs number {}.", Index,
        CxtRef.TorchOutputs.size());
    return ErrNo::InvalidArgument;
  }
  torch::Tensor OutTensor =
      CxtRef.TorchOutputs[Index].to(at::kCPU).toType(torch::kFloat32);
  float *TensorBuffer = OutTensor.data_ptr<float>();

  size_t BlobSize = 1;
  for (auto I : OutTensor.sizes()) {
    BlobSize *= I;
  }
  uint32_t BytesToWrite =
      std::min(static_cast<size_t>(BlobSize * 4), OutBuffer.size());
  std::copy_n(reinterpret_cast<const uint8_t *>(TensorBuffer), BytesToWrite,
              OutBuffer.data());
  BytesWritten = BytesToWrite;
  return ErrNo::Success;
}

Expect<ErrNo> compute(WasiNNEnvironment &Env, uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  if (CxtRef.TorchInputs.size() == 0) {
    spdlog::error("[WASI-NN] Input is not set!");
    return ErrNo::InvalidArgument;
  }
  for (size_t I = 0; I < CxtRef.TorchInputs.size(); I++) {
    torch::jit::IValue InTensor = CxtRef.TorchInputs[I];
    if (InTensor.isNone()) {
      spdlog::error("[WASI-NN] Input [{}] is not set!", I);
      return ErrNo::InvalidArgument;
    }
  }
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  torch::jit::IValue RawOutput =
      GraphRef.TorchModel.forward(CxtRef.TorchInputs);
  // TODO: more output type should be supported here
  if (RawOutput.isTensorList()) {
    auto OutTensors = RawOutput.toTensorVector();
    for (auto &OneOf : OutTensors) {
      CxtRef.TorchOutputs.push_back(OneOf.clone());
    }
  } else if (RawOutput.isTuple()) {
    auto OutTensorsTuple = RawOutput.toTuple()->elements();
    for (auto &OneOf : OutTensorsTuple) {
      CxtRef.TorchOutputs.push_back(OneOf.toTensor().clone());
    }
  } else if (RawOutput.isTensor()) {
    auto OutTensor = RawOutput.toTensor();
    CxtRef.TorchOutputs.push_back(OutTensor.clone());
  } else {
    spdlog::error("[WASI-NN] PyTorch backend only supports output a tensor, "
                  "a list of tensor or a tuple of tensor");
    return ErrNo::InvalidArgument;
  }
  return ErrNo::Success;
}
#else
namespace {
Expect<ErrNo> reportBackendNotSupported() noexcept {
  spdlog::error("[WASI-NN] PyTorch backend is not built. use "
                "-WASMEDGE_PLUGIN_WASI_NN_BACKEND=\"PyTorch\" to build it.");
  return ErrNo::InvalidArgument;
}
} // namespace

Expect<ErrNo> load(WasiNNEnvironment &, Span<const Span<uint8_t>>, Device,
                   uint32_t &) noexcept {
  return reportBackendNotSupported();
}
Expect<ErrNo> initExecCtx(WasiNNEnvironment &, uint32_t, uint32_t &) noexcept {
  return reportBackendNotSupported();
}
Expect<ErrNo> setInput(WasiNNEnvironment &, uint32_t, uint32_t,
                       const TensorData &) noexcept {
  return reportBackendNotSupported();
}
Expect<ErrNo> getOutput(WasiNNEnvironment &, uint32_t, uint32_t, Span<uint8_t>,
                        uint32_t &) noexcept {
  return reportBackendNotSupported();
}
Expect<ErrNo> compute(WasiNNEnvironment &, uint32_t) noexcept {
  return reportBackendNotSupported();
}

#endif
} // namespace WasmEdge::Host::WASINN::PyTorch
