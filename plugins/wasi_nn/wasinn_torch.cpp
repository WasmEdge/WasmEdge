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
      spdlog::error("[WASI-NN] Torch: CUDA Unavailable. Please check if the "
                    "installed Torch version or platform supports CUDA."sv);
      return ErrNo::InvalidArgument;
    }
    TorchDevice = at::kCUDA;
    return ErrNo::Success;
  }

  spdlog::error("[WASI-NN] Torch: Unknown target device. We currently support "
                "only CPU and GPU targets."sv);
  return ErrNo::InvalidArgument;
}

Expect<ErrNo> TorchScript::loadFromBinary(std::istream &In, Device Device) {
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
    spdlog::error(
        "[WASI-NN] Torch: The output can only be one of the following tensor "
        "types: a tensor, a list of tensors, or a tuple of tensors."sv);
    return ErrNo::InvalidArgument;
  }
  return ErrNo::Success;
}

AOTInductor::AOTInductor() : TorchModel(nullptr) {
#if defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI == 1
  spdlog::warn(
      "[WASI-NN] Torch: AOTInductor build by pip default is not supported in "
      "_GLIBCXX_USE_CXX11_ABI=1. Please rebuild the WasmEdge with "
      "_GLIBCXX_USE_CXX11_ABI=0."sv);
#endif
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
    spdlog::error("[WASI-NN] Torch: Please rebuild the plugin with AOTInductor "
                  "CUDA support."sv);
    return ErrNo::InvalidArgument;
#endif
  }

  spdlog::error("[WASI-NN] Torch: Unknown target device. We currently support "
                "only CPU and GPU targets."sv);
  return ErrNo::InvalidArgument;
}

Expect<ErrNo> AOTInductor::loadFromBinary(std::istream &, Device) {
  spdlog::error(
      "[WASI-NN] Torch: AOTInductor can not load by binary data. Please "
      "pass the share library name (*.so) in nn-preload"sv);
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
    spdlog::error("[WASI-NN] Torch: Please rebuild the plugin with AOTInductor "
                  "CUDA support."sv);
    return ErrNo::InvalidArgument;
#endif
  } else {
    spdlog::error("[WASI-NN] Torch: Can not load the AOTInductor."sv);
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

PyModelBackend guessPyModelBackendType(const std::string_view &Model) {
  // TODO: Add more model type detection when we supporet more OS.
  // ex .dll, .dylib, etc.
  if (Model.substr(0, 8) == "preload:"sv) {
    if (Model.substr(Model.size() - 3, 3) == ".so"sv) {
      // AOTInductor only accept the shared library.
      return PyModelBackend::AOTInductor;
    }
  }

  // ELF Header: 0x7f 'E' 'L' 'F'
  if (Model.substr(0, 4) == "\x7f\x45\x4c\x46"sv) {
    return PyModelBackend::AOTInductor;
  }

  // Fall back to TorchScript if the model type is not set.
  // This keep the compatibility with the old version.
  return PyModelBackend::TorchScript;
}

Expect<ErrNo> load(WasiNNEnvironment &Env, Span<const Span<uint8_t>> Builders,
                   Device Device, uint32_t &GraphId) noexcept {
  // The graph builder length must be 1.
  if (Builders.size() != 1) {
    spdlog::error("[WASI-NN] Torch: Wrong GraphBuilder Length {:d}, expect 1"sv,
                  Builders.size());
    return ErrNo::InvalidArgument;
  }

  auto Weight = Builders[0];
  // Add a new graph.
  uint32_t GId = Env.newGraph(Backend::PyTorch);
  auto &GraphRef = Env.NNGraph[GId].get<Graph>();

  // Load the model from the binary data.
  // Note: Pytorch use try catch to handle the error.
  try {
    const std::string_view BinModel(reinterpret_cast<char *>(Weight.data()),
                                    Weight.size());
    PyModelBackend ModelType = guessPyModelBackendType(BinModel);

    if (ModelType == PyModelBackend::TorchScript) {
      GraphRef.Model = new TorchScript();
    } else if (ModelType == PyModelBackend::AOTInductor) {
      GraphRef.Model = new AOTInductor();
    } else {
      spdlog::error("[WASI-NN] Torch: Unknown model type."sv);
      return ErrNo::InvalidArgument;
    }

    if (BinModel.substr(0, 8) == "preload:"sv) {
      const std::string ModelFilePath(BinModel.substr(8));
      GraphRef.Model->loadFromPath(ModelFilePath, Device);
    } else {
      std::istringstream BinRead{std::string(BinModel)};
      // std::istringstream BinRead(BinModel); // Need C++26...
      GraphRef.Model->loadFromBinary(BinRead, Device);
    }
  } catch (const c10::Error &e) {
    spdlog::error("[WASI-NN] Torch: Failed when load the TorchScript model."sv);
    Env.NNGraph.pop_back();
    return ErrNo::InvalidArgument;
  }

  GraphId = GId;
  Env.NNGraph[GId].setReady();
  return ErrNo::Success;
}

Expect<ErrNo> initExecCtx(WasiNNEnvironment &Env, uint32_t GraphId,
                          uint32_t &ContextId) noexcept {
  ContextId = Env.newContext(GraphId, Env.NNGraph[GraphId]);
  Env.NNContext[ContextId].setReady();
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
        "[WASI-NN] Torch: Only F32 inputs and outputs are supported for now."sv);
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
        "[WASI-NN] Torch: The output index {} exceeds the outputs number {}."sv,
        Index, CxtRef.TorchOutputs.size());
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
    spdlog::error("[WASI-NN] Torch: Input is not set!"sv);
    return ErrNo::InvalidArgument;
  }
  for (size_t I = 0; I < CxtRef.TorchInputs.size(); I++) {
    torch::jit::IValue InTensor = CxtRef.TorchInputs[I];
    if (InTensor.isNone()) {
      spdlog::error("[WASI-NN] Torch: Input [{}] is not set!"sv, I);
      return ErrNo::InvalidArgument;
    }
  }
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  return GraphRef.Model->run(CxtRef.TorchInputs, CxtRef.TorchOutputs);
}

Expect<ErrNo> unload(WasiNNEnvironment &Env, uint32_t GraphId) noexcept {
  auto &GraphRef = Env.NNGraph[GraphId].get<Graph>();
  if (GraphRef.Model) {
    delete GraphRef.Model;
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
Expect<ErrNo> unload(WasiNNEnvironment &, uint32_t) noexcept {
  return reportBackendNotSupported();
}

#endif
} // namespace WasmEdge::Host::WASINN::PyTorch
