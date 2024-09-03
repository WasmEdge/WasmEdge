// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "wasinn_torch.h"
#include "wasinnenv.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TORCH
#include <torch/torch.h>
#endif

namespace WasmEdge::Host::WASINN::PyTorch {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_TORCH

Expect<ErrNo> TorchScript::setDevice(Device Device) {
  if (Device == Device::CPU) {
    TorchDevice = at::kCPU;
    return ErrNo::Success;
  } else if (Device == Device::GPU) {
    if (!torch::cuda::is_available()) {
      spdlog::error(
          "[WASI-NN] CUDA Unavailable, platform Cannot support GPU target.");
      return ErrNo::InvalidArgument;
    }
    TorchDevice = at::kCUDA;
    return ErrNo::Success;
  }

  spdlog::error("[WASI-NN] PyTorch Only support CPU and GPU target.");
  return ErrNo::InvalidArgument;
}

Expect<ErrNo> TorchScript::loadFromBiary(std::istream &In, Device Device) {
  if (auto Err = setDevice(Device); Err != ErrNo::Success) {
    return Err;
  }
  TorchModel = torch::jit::load(In);
  return ErrNo::Success;
}

Expect<ErrNo> TorchScript::loadFromPath(const std::string &Path,
                                        Device Device) {
  if (auto Err = setDevice(Device); Err != ErrNo::Success) {
    return Err;
  }
  TorchModel = torch::jit::load(Path);
  return ErrNo::Success;
}

Expect<ErrNo> TorchScript::run(std::vector<at::Tensor> In,
                               std::vector<at::Tensor> &Out) {
  std::vector<torch::jit::IValue> Inputs;
  std::vector<at::Tensor> Outputs;
  for (auto &OneOf : In) {
    Inputs.push_back(OneOf);
  }
  auto RawOutput = TorchModel.forward(Inputs);
  if (RawOutput.isTensorList()) {
    auto OutTensors = RawOutput.toTensorVector();
    for (auto &OneOf : OutTensors) {
      Out.push_back(OneOf.clone());
    }
  } else if (RawOutput.isTuple()) {
    auto OutTensorsTuple = RawOutput.toTuple()->elements();
    for (auto &OneOf : OutTensorsTuple) {
      Out.push_back(OneOf.toTensor().clone());
    }
  } else if (RawOutput.isTensor()) {
    auto OutTensor = RawOutput.toTensor();
    Out.push_back(OutTensor.clone());
  } else {
    spdlog::error("[WASI-NN] PyTorch backend only supports output a tensor, "
                  "a list of tensor or a tuple of tensor");
    return ErrNo::InvalidArgument;
  }
  return ErrNo::Success;
}

Expect<ErrNo> AOTInductor::setDevice(Device Device) {
  if (Device == Device::CPU) {
    TorchDevice = at::kCPU;
    return ErrNo::Success;
  } else if (Device == Device::GPU) {
#ifdef TORCHAOTI_USE_CUDA
    TorchDevice = at::kCUDA;
    return ErrNo::Success;
#else
    spdlog::error(
        "[WASI-NN] Please rebuild the plugin with AOTInductor CUDA support.");
    return ErrNo::InvalidArgument;
#endif
  }

  spdlog::error("[WASI-NN] AOTInductor Only support CPU and GPU target.");
  return ErrNo::InvalidArgument;
}

Expect<ErrNo> AOTInductor::loadFromBiary(std::istream &, Device) {
  spdlog::error("[WASI-NN] AOTInductor can not load by binary data. Please "
                "pass the share library name (*.so) in nn-preload");
  return ErrNo::InvalidArgument;
}

Expect<ErrNo> AOTInductor::loadFromPath(const std::string &Path,
                                        Device Device) {
  if (auto Err = setDevice(Device); Err != ErrNo::Success) {
    return Err;
  }
  if (TorchDevice == at::kCPU) {
    TorchModel = new torch::inductor::AOTIModelContainerRunnerCpu(Path.c_str());
  } else if (TorchDevice == at::kCUDA) {
#ifdef TORCHAOTI_USE_CUDA
    TorchModel =
        new torch::inductor::AOTIModelContainerRunnerCuda(Path.c_str());
#else
    spdlog::error(
        "[WASI-NN] Please rebuild the plugin with AOTInductor CUDA support.");
    return ErrNo::InvalidArgument;
#endif
  } else {
    spdlog::error("[WASI-NN] Can not load the AOTInductor.");
    return ErrNo::InvalidArgument;
  }
  return ErrNo::Success;
}

Expect<ErrNo> AOTInductor::run(std::vector<at::Tensor> In,
                               std::vector<at::Tensor> &Out) {
  std::vector<at::Tensor> RawOutput = TorchModel->run(In);

  for (auto &OneOf : RawOutput) {
    Out.push_back(OneOf.clone());
  }
  return ErrNo::Success;
}

PyModelBackend GuessPyModelBackendType(const std::string_view &Model) {
  if (Model.substr(0, 8) == "preload:"sv) {
    if (Model.substr(Model.size() - 3, 3) == ".so"sv) {
      // AOTInductor only accept the shared library.
      return PyModelBackend::AOTInductor;
    }
  }

  // Fall back to TorchScript if the model type is not set.
  // This keep the compatibility with the old version.
  return PyModelBackend::TorchScript;
}

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

  // Load the model from the binary data.
  // Note: Pytorch use try catch to handle the error.
  try {
    const std::string_view BinModel(reinterpret_cast<char *>(Weight.data()),
                                    Weight.size());
    PyModelBackend ModelType = GuessPyModelBackendType(BinModel);

    if (ModelType == PyModelBackend::TorchScript) {
      GraphRef.Model = new TorchScript();
    } else if (ModelType == PyModelBackend::AOTInductor) {
      GraphRef.Model = new AOTInductor();
    } else {
      spdlog::error("[WASI-NN] Unknown model type.");
      return ErrNo::InvalidArgument;
    }

    if (BinModel.substr(0, 8) == "preload:"sv) {
      const std::string ModelFilePath(BinModel.substr(8));
      GraphRef.Model->loadFromPath(ModelFilePath, Device);
    } else {
      std::istringstream BinRead{std::string(BinModel)};
      // std::istringstream BinRead(BinModel); // Need C++26...
      GraphRef.Model->loadFromBiary(BinRead, Device);
    }
  } catch (const c10::Error &e) {
    spdlog::error("[WASI-NN] Failed when load the TorchScript model.");
    Env.NNGraph.pop_back();
    return ErrNo::InvalidArgument;
  }

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
          .to(GraphRef.Model->getDevice());

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
  return GraphRef.Model->run(CxtRef.TorchInputs, CxtRef.TorchOutputs);
}

Expect<ErrNo> unload(WasiNNEnvironment &Env, uint32_t GraphId) noexcept {
  auto &GraphRef = Env.NNGraph[GraphId].get<Graph>();
  delete GraphRef.Model;
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
Expect<ErrNo> unload(WasiNNEnvironment &, uint32_t) noexcept {
  return reportBackendNotSupported();
}

#endif
} // namespace WasmEdge::Host::WASINN::PyTorch
