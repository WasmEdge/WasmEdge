#include "neuralspeed.h"
#include "wasinnenv.h"

namespace WasmEdge::Host::WASINN::NeuralSpeed {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_NEURAL_SPEED
Expect<WASINN::ErrNo> load(WASINN::WasiNNEnvironment &Env,
                           Span<const Span<uint8_t>> Builders, WASINN::Device,
                           uint32_t &) noexcept {
  // Add a new graph.
  Env.NNGraph.emplace_back(Backend::GGML);
  auto &GraphRef = Env.NNGraph.back().get<Graph>();

  // Initialize the plugin parameters.
  GraphRef.EnableDebugLog = true;
  // Handle the model path.
  auto Weight = Builders[0];
  const std::string BinModel(reinterpret_cast<char *>(Weight.data()),
                             Weight.size());
  std::string ModelFilePath;
  if (BinModel.substr(0, 8) == "preload:") {
    ModelFilePath = BinModel.substr(8);
  } else {
    if (GraphRef.EnableDebugLog) {
      spdlog::info(
          "[WASI-NN][Debug] Neural speed: Model path not found in nn-preload, "
          "write model into a tmpfile."sv);
    }
    // TODO: pass the model directly to ggml
    // Write neural speed model to file.
    ModelFilePath = "neural-speed-model.bin"sv;
    std::ofstream TempFile(ModelFilePath);
    if (!TempFile) {
      spdlog::error(
          "[WASI-NN] Neural speed: Failed to create the temporary file. "
          "Currently, our workaround involves creating a temporary model "
          "file named \"ggml-model.bin\" and passing this filename as a "
          "parameter to the ggml llama library."sv);
      Env.NNGraph.pop_back();
      return ErrNo::InvalidArgument;
    }
    TempFile << BinModel;
    TempFile.close();
    if (GraphRef.EnableDebugLog) {
      spdlog::info(
          "[WASI-NN][Debug] Neural speed: Write model into a tmpfile...Done"sv);
    }
  }
  if (GraphRef.EnableDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] Neural speed: Finished handling model path."sv);
  }

  // Create Model class
  PyObject *moduleName = PyUnicode_FromString("neural_speed");
  GraphRef.NeuralSpeedModule = PyImport_Import(moduleName);
  Py_DECREF(moduleName);
  if (GraphRef.NeuralSpeedModule == nullptr) {
    spdlog::error(
        "[WASI-NN] neural speed backend: Can not find neural speed library."sv);
    return WASINN::ErrNo::RuntimeError;
  }
  GraphRef.ModelClass =
      PyObject_GetAttrString(GraphRef.NeuralSpeedModule, "Model");
  if (GraphRef.ModelClass == nullptr ||
      !PyCallable_Check(GraphRef.ModelClass)) {
    spdlog::error(
        "[WASI-NN] neural speed backend: Can not find Model class in neural speed."sv);
    return WASINN::ErrNo::RuntimeError;
  }
  GraphRef.Model = PyObject_CallObject(GraphRef.ModelClass, NULL);
  PyObject *LoadResult = PyObject_CallMethod(GraphRef.Model, "init_from_bin",
                                             "llama", ModelFilePath);
  if (LoadResult == nullptr) {
    spdlog::error("[WASI-NN] neural speed backend: Load model error."sv);
    return WASINN::ErrNo::RuntimeError;
  }
  Py_XDECREF(LoadResult);

  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> initExecCtx(WasiNNEnvironment &Env, uint32_t GraphId,
                                  uint32_t &ContextId) noexcept {
  Env.NNContext.emplace_back(GraphId, Env.NNGraph[GraphId]);
  ContextId = Env.NNContext.size() - 1;
  return ErrNo::Success;
}

Expect<WASINN::ErrNo> setInput(WasiNNEnvironment &Env, uint32_t ContextId,
                               uint32_t, const TensorData &Tensor) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] Neural speed backend: setInput"sv);
  }

  // Set the input.
  if (Tensor.Tensor.size() % sizeof(long long int) != 0) {
    spdlog::error("[WASI-NN] neural speed backend: Input tensor size is not a "
                  "multiple of "
                  "4 bytes."sv);
    return WASINN::ErrNo::InvalidArgument;
  }
  std::vector<long long int> Prompt{
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
    spdlog::info("[WASI-NN][Debug] Neural speed backend: getOutput"sv);
  }
  std::string stmp(reinterpret_cast<const char *>(CxtRef.Outputs.data()),
                   CxtRef.Outputs.size() * sizeof(long long int));
  std::copy_n(stmp.data(), stmp.length(), OutBuffer.data());
  BytesWritten = stmp.length();
  return WASINN::ErrNo::Success;
}
Expect<WASINN::ErrNo> compute(WasiNNEnvironment &Env,
                              uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN][Debug] Neural speed backend: compute"sv);
  }
  if (CxtRef.Inputs.size() == 0) {
    spdlog::error("[WASI-NN] Neural speed backend: Llama input is not set!"sv);
    return ErrNo::InvalidArgument;
  }
  CxtRef.Outputs.clear();
  PyObject *TensorList = PyList_New(0);
  for (size_t i = 0; i < CxtRef.Inputs.size(); ++i) {
    PyObject *num = PyLong_FromLong(CxtRef.Inputs[i]);
    PyList_Append(TensorList, num);
    Py_DECREF(num);
  }
  PyObject *TmpArg = PyList_New(0);
  PyList_Append(TmpArg, TensorList);
  Py_DECREF(TensorList);
  PyObject *torchModule = PyImport_ImportModule("torch");
  if (torchModule == nullptr) {
    spdlog::error(
        "[WASI-NN] neural speed backend: Can not find torch library."sv);
    return WASINN::ErrNo::RuntimeError;
  }
  PyObject *LongTensorFunc = PyObject_GetAttrString(torchModule, "LongTensor");
  PyObject *LongTensorArgs = PyTuple_Pack(1, TmpArg);
  PyObject *LongTensor = PyObject_CallObject(LongTensorFunc, LongTensorArgs);
  Py_DECREF(torchModule);
  Py_DECREF(LongTensorFunc);
  Py_DECREF(TmpArg);
  Py_DECREF(LongTensorArgs);
  if (LongTensor == nullptr) {
    spdlog::error(
        "[WASI-NN] neural speed backend: Input transfer tensor failed."sv);
    return WASINN::ErrNo::InvalidArgument;
  }
  PyObject *GenerateArgs = PyTuple_Pack(1, LongTensor);
  PyObject *result = PyObject_CallMethodObjArgs(
      GraphRef.Model, PyUnicode_FromString("generate"), GenerateArgs, NULL);
  if (result == nullptr) {
    spdlog::error(
        "[WASI-NN] neural speed backend: Neural Speed runtime error."sv);
    return WASINN::ErrNo::RuntimeError;
  }
  if (PyList_Check(result)) {
    Py_ssize_t outerSize = PyList_Size(result);
    for (Py_ssize_t i = 0; i < outerSize; ++i) {
      PyObject *innerList = PyList_GetItem(result, i);
      if (PyList_Check(innerList)) {
        std::vector<long long int> innerVec;
        Py_ssize_t innerSize = PyList_Size(innerList);
        for (Py_ssize_t j = 0; j < innerSize; ++j) {
          PyObject *num = PyList_GetItem(innerList, j);
          if (PyLong_Check(num)) {
            innerVec.push_back(PyLong_AsLong(num));
          }
        }
        CxtRef.Outputs = innerVec;
      }
    }
  }
  Py_DECREF(result);
  Py_DECREF(GenerateArgs);
  Py_DECREF(LongTensor);
  return WASINN::ErrNo::Success;
}
#else
namespace {
Expect<WASINN::ErrNo> reportBackendNotSupported() noexcept {
  spdlog::error("[WASI-NN] Neural speed backend is not supported.");
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
} // namespace WasmEdge::Host::WASINN::NeuralSpeed
