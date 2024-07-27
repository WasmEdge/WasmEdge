// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "neuralspeed.h"
#include "wasinnenv.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_NEURAL_SPEED
#include "simdjson.h"

#if !defined(_WIN32) && !defined(_WIN64) && !defined(__WIN32__) &&             \
    !defined(__TOS_WIN__) && !defined(__WINDOWS__)
#include <dlfcn.h>
#endif
#include <chrono>
#include <time.h>
#endif

namespace WasmEdge::Host::WASINN::NeuralSpeed {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_NEURAL_SPEED

#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) ||                \
    defined(__TOS_WIN__) || defined(__WINDOWS__)
HINSTANCE SharedLib = LoadLibrary(PYTHON_LIB_PATH);
#else
void *SharedLib = dlopen(PYTHON_LIB_PATH, RTLD_GLOBAL | RTLD_NOW);
#endif

void printImformation(Graph &GraphRef, Context &CxtRef) {
  spdlog::info("[WASI-NN] Neural speed backend: Number of input tokens: {}"sv,
               CxtRef.Inputs.size());
  spdlog::info("[WASI-NN] Neural speed backend: Number of Output tokens: {}"sv,
               CxtRef.Outputs.size());
  spdlog::info("[WASI-NN] Neural speed backend: Load time: {} ms"sv,
               GraphRef.LoadTime);
  spdlog::info("[WASI-NN] Neural speed backend: Compute time: {} ms "sv,
               GraphRef.ComputeTime);
}

Expect<WASINN::ErrNo> load(WASINN::WasiNNEnvironment &Env,
                           Span<const Span<uint8_t>> Builders, WASINN::Device,
                           uint32_t &GraphId) noexcept {
  // Add a new graph.
  Env.NNGraph.emplace_back(Backend::NeuralSpeed);
  auto &GraphRef = Env.NNGraph.back().get<Graph>();
  const auto StartTime = std::chrono::steady_clock::now();
  // Initialize the plugin parameters.
  GraphRef.EnableDebugLog = false;
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN] Neural speed backend: Load."sv);
  }

  if (Builders.size() > 1) {
    std::string Metadata = std::string(
        reinterpret_cast<char *>(Builders[1].data()), Builders[1].size());
    simdjson::dom::parser Parser;
    simdjson::dom::element Doc;
    auto ParseError = Parser.parse(Metadata).get(Doc);
    if (ParseError) {
      spdlog::error("[WASI-NN] Neural speed backend: Parse metadata error"sv);
      Env.NNGraph.pop_back();
      return ErrNo::InvalidEncoding;
    }
    if (Doc.at_key("model_type").error() == simdjson::SUCCESS) {
      std::string_view ModelType;
      auto Err = Doc["model_type"].get<std::string_view>().get(ModelType);
      if (Err) {
        spdlog::error(
            "[WASI-NN] Neural speed backend: Unable to retrieve the model_type option."sv);
        Env.NNGraph.pop_back();
        return ErrNo::InvalidArgument;
      }
      GraphRef.ModelType = ModelType;
    }
  }

  // Handle the model path.
  auto Weight = Builders[0];
  const std::string BinModel(reinterpret_cast<char *>(Weight.data()),
                             Weight.size());
  spdlog::info("[WASI-NN] Neural speed: BinModel: {}"sv, BinModel.size());
  std::string ModelFilePath;
  if (BinModel.substr(0, 8) == "preload:") {
    ModelFilePath = BinModel.substr(8);
  } else {
    if (GraphRef.EnableDebugLog) {
      spdlog::info(
          "[WASI-NN] Neural speed: Model path not found in nn-preload, "
          "write model into a tmpfile."sv);
    }
    // Write neural speed model to file.
    ModelFilePath = "neural-speed-model.bin"sv;
    std::ofstream TempFile(ModelFilePath, std::ios::binary);
    TempFile.imbue(
        std::locale(TempFile.getloc(), new std::codecvt_utf8<wchar_t>));
    if (!TempFile) {
      spdlog::error(
          "[WASI-NN] Neural speed: Failed to create the temporary file. "
          "Currently, our workaround involves creating a temporary model."sv);
      Env.NNGraph.pop_back();
      return ErrNo::InvalidArgument;
    }
    TempFile << BinModel;
    TempFile.close();
    if (GraphRef.EnableDebugLog) {
      spdlog::info(
          "[WASI-NN] Neural speed: Write model into a tmpfile...Done"sv);
    }
  }
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN] Neural speed: Finished handling model path."sv);
  }

  // Create Model class
  if (!Py_IsInitialized()) {
    Py_Initialize();
  }
  if (GraphRef.NeuralSpeedModule == nullptr) {
    GraphRef.NeuralSpeedModule = PyImport_ImportModule("neural_speed");
  }
  if (GraphRef.NeuralSpeedModule == nullptr) {
    PyErr_Print();
    spdlog::error(
        "[WASI-NN] neural speed backend: Can not find neural speed library."sv);
    Env.NNGraph.pop_back();
    return WASINN::ErrNo::RuntimeError;
  }
  if (GraphRef.ModelClass == nullptr) {
    GraphRef.ModelClass =
        PyObject_GetAttrString(GraphRef.NeuralSpeedModule, "Model");
  }
  if (GraphRef.ModelClass == nullptr ||
      !PyCallable_Check(GraphRef.ModelClass)) {
    spdlog::error(
        "[WASI-NN] neural speed backend: Can not find Model class in neural speed."sv);
    Env.NNGraph.pop_back();
    return WASINN::ErrNo::RuntimeError;
  }
  GraphRef.Model = PyObject_CallObject(GraphRef.ModelClass, NULL);
  if (GraphRef.Model == nullptr) {
    spdlog::error("[WASI-NN] neural speed backend: Load model error."sv);
    Env.NNGraph.pop_back();
    return WASINN::ErrNo::InvalidArgument;
  }
  PyObject *LoadResult =
      PyObject_CallMethod(GraphRef.Model, "init_from_bin", "(ss)",
                          GraphRef.ModelType.c_str(), ModelFilePath.c_str());
  if (LoadResult == nullptr) {
    spdlog::error("[WASI-NN] neural speed backend: Load model error."sv);
    Py_XDECREF(GraphRef.Model);
    Env.NNGraph.pop_back();
    return WASINN::ErrNo::InvalidArgument;
  }
  Py_XDECREF(LoadResult);
  GraphRef.LoadTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::steady_clock::now() - StartTime)
                          .count();

  // Store the loaded graph.
  GraphId = Env.NNGraph.size() - 1;

  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> initExecCtx(WasiNNEnvironment &Env, uint32_t GraphId,
                                  uint32_t &ContextId) noexcept {
  if (!Py_IsInitialized()) {
    spdlog::error(
        "[WASI-NN] Neural speed backend: Model has been realse, please reload it."sv);
    return WASINN::ErrNo::RuntimeError;
  }
  Env.NNContext.emplace_back(GraphId, Env.NNGraph[GraphId]);
  ContextId = Env.NNContext.size() - 1;
  return ErrNo::Success;
}

Expect<WASINN::ErrNo> setInput(WasiNNEnvironment &Env, uint32_t ContextId,
                               uint32_t, const TensorData &Tensor) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  if (!Py_IsInitialized()) {
    spdlog::error(
        "[WASI-NN] Neural speed backend: Model has been realse, please reload it."sv);
    return WASINN::ErrNo::RuntimeError;
  }
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN] Neural speed backend: setInput"sv);
  }

  // Set the input.
  if (Tensor.Tensor.size() % sizeof(long long int) != 0) {
    spdlog::error("[WASI-NN] neural speed backend: Input tensor size is not a "
                  "multiple of "
                  "4 bytes."sv);
    return WASINN::ErrNo::InvalidArgument;
  }
  const std::vector<long long int> Prompt{
      reinterpret_cast<const long int *>(Tensor.Tensor.data()),
      reinterpret_cast<const long *>(Tensor.Tensor.data() +
                                     Tensor.Tensor.size())};
  CxtRef.Inputs.clear();
  CxtRef.Inputs = Prompt;

  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> getOutput(WasiNNEnvironment &Env, uint32_t ContextId,
                                uint32_t, Span<uint8_t> OutBuffer,
                                uint32_t &BytesWritten) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN] Neural speed backend: getOutput"sv);
  }
  std::string StringTmp(reinterpret_cast<const char *>(CxtRef.Outputs.data()),
                        CxtRef.Outputs.size() * sizeof(long long int));
  std::copy_n(StringTmp.data(), StringTmp.length(), OutBuffer.data());
  BytesWritten = StringTmp.length();
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> compute(WasiNNEnvironment &Env,
                              uint32_t ContextId) noexcept {
  if (!Py_IsInitialized()) {
    spdlog::error(
        "[WASI-NN]Neural speed backend: Model has been realse, please reload it."sv);
    return WASINN::ErrNo::RuntimeError;
  }
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  const auto StartTime = std::chrono::steady_clock::now();
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN] Neural speed backend: compute"sv);
  }
  if (CxtRef.Inputs.size() == 0) {
    spdlog::error("[WASI-NN] Neural speed backend: Llama input is not set!"sv);
    return ErrNo::InvalidArgument;
  }
  CxtRef.Outputs.clear();
  PyObject *TensorList = PyList_New(0);
  for (size_t Cnt = 0; Cnt < CxtRef.Inputs.size(); ++Cnt) {
    PyObject *Num = PyLong_FromLong(CxtRef.Inputs[Cnt]);
    PyList_Append(TensorList, Num);
    Py_DECREF(Num);
  }
  PyObject *TmpArg = PyList_New(0);
  PyList_Append(TmpArg, TensorList);
  PyObject *TorchModule = PyImport_ImportModule("torch");
  if (TorchModule == nullptr) {
    spdlog::error(
        "[WASI-NN] neural speed backend: Can not find torch library."sv);
    return WASINN::ErrNo::RuntimeError;
  }
  PyObject *LongTensorFunc = PyObject_GetAttrString(TorchModule, "LongTensor");
  PyObject *LongTensorArgs = PyTuple_Pack(1, TmpArg);
  PyObject *LongTensor = PyObject_CallObject(LongTensorFunc, LongTensorArgs);
  Py_DECREF(TensorList);
  Py_DECREF(TorchModule);
  Py_DECREF(LongTensorFunc);
  Py_DECREF(TmpArg);
  Py_DECREF(LongTensorArgs);
  if (LongTensor == nullptr) {
    spdlog::error(
        "[WASI-NN] neural speed backend: Input transfer tensor failed."sv);
    return WASINN::ErrNo::InvalidArgument;
  }
  PyObject *GenerateString = PyUnicode_FromString("generate");
  PyObject *Result = PyObject_CallMethodObjArgs(GraphRef.Model, GenerateString,
                                                LongTensor, NULL);
  Py_DECREF(GenerateString);
  if (Result == nullptr) {
    PyErr_Print();
    spdlog::error(
        "[WASI-NN] neural speed backend: Neural Speed runtime error."sv);
    return WASINN::ErrNo::RuntimeError;
  }
  if (PyList_Check(Result)) {
    const Py_ssize_t OuterSize = PyList_Size(Result);
    for (Py_ssize_t OutterCnt = 0; OutterCnt < OuterSize; ++OutterCnt) {
      PyObject *InnerList = PyList_GetItem(Result, OutterCnt);
      if (PyList_Check(InnerList)) {
        std::vector<long long int> InnerVec;
        const Py_ssize_t InnerSize = PyList_Size(InnerList);
        for (Py_ssize_t InnerCnt = 0; InnerCnt < InnerSize; ++InnerCnt) {
          PyObject *Num = PyList_GetItem(InnerList, InnerCnt);
          if (PyLong_Check(Num)) {
            InnerVec.push_back(PyLong_AsLong(Num));
          }
        }
        CxtRef.Outputs = InnerVec;
      }
    }
  }
  Py_DECREF(Result);
  Py_DECREF(LongTensor);
  GraphRef.ComputeTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - StartTime)
                             .count();
  if (GraphRef.EnableDebugLog) {
    printImformation(GraphRef, CxtRef);
  }
  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> unload(WASINN::WasiNNEnvironment &Env,
                             uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN] Neural speed backend: start unload."sv);
  }
  if (Py_IsInitialized()) {
    Py_XDECREF(GraphRef.Model);
    Py_XDECREF(GraphRef.ModelClass);
    Py_XDECREF(GraphRef.NeuralSpeedModule);
    GraphRef.ModelClass = nullptr;
    GraphRef.NeuralSpeedModule = nullptr;
    Py_Finalize();
  }
  return WASINN::ErrNo::Success;
}

#else
namespace {
Expect<WASINN::ErrNo> reportBackendNotSupported() noexcept {
  spdlog::error(
      "[WASI-NN] Neural speed backend is not built. use "
      "-WASMEDGE_PLUGIN_WASI_NN_BACKEND=\"NeuralSpeed\" to build it."sv);
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
Expect<WASINN::ErrNo> unload(WASINN::WasiNNEnvironment &, uint32_t) noexcept {
  return reportBackendNotSupported();
}
#endif
} // namespace WasmEdge::Host::WASINN::NeuralSpeed
