// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "chattts.h"
#include "wasinnenv.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_CHATTTS
#include "simdjson.h"

#if !defined(_WIN32) && !defined(_WIN64) && !defined(__WIN32__) &&             \
    !defined(__TOS_WIN__) && !defined(__WINDOWS__)
#include <dlfcn.h>
#endif
#include <chrono>
#include <time.h>
#endif

namespace WasmEdge::Host::WASINN::ChatTTS {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_CHATTTS
#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) ||                \
    defined(__TOS_WIN__) || defined(__WINDOWS__)
HINSTANCE SharedLib = LoadLibrary(PYTHON_LIB_PATH);
#else
void *SharedLib = dlopen(PYTHON_LIB_PATH, RTLD_GLOBAL | RTLD_NOW);
#endif
Expect<WASINN::ErrNo> load(WASINN::WasiNNEnvironment &Env,
                           Span<const Span<uint8_t>>, WASINN::Device,
                           uint32_t &GraphId) noexcept {
  // Add a new graph.
  Env.NNGraph.emplace_back(Backend::ChatTTS);
  auto &GraphRef = Env.NNGraph.back().get<Graph>();
  // Initialize the plugin parameters.
  GraphRef.EnableDebugLog = true;
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN] ChatTTS backend: Load."sv);
  }

  // if (Builders.size() > 1) {
  //   std::string Metadata = std::string(
  //       reinterpret_cast<char *>(Builders[1].data()), Builders[1].size());
  //   simdjson::dom::parser Parser;
  //   simdjson::dom::element Doc;
  //   auto ParseError = Parser.parse(Metadata).get(Doc);
  //   if (ParseError) {
  //     spdlog::error("[WASI-NN] ChatTTS backend: Parse metadata error"sv);
  //     Env.NNGraph.pop_back();
  //     return ErrNo::InvalidEncoding;
  //   }
  //   // TODO handle metadata
  // }

  // Create Model class
  if (!Py_IsInitialized()) {
    Py_Initialize();
  }
  GraphRef.ChatTTSModule = PyImport_Import(PyUnicode_FromString("ChatTTS"));
  if (GraphRef.ChatTTSModule == nullptr) {
    PyErr_Print();
    spdlog::error("[WASI-NN] ChatTTS backend: Can not find ChatTTS library."sv);
    Env.NNGraph.pop_back();
    return WASINN::ErrNo::RuntimeError;
  }
  PyObject *ChatFunction =
      PyObject_GetAttrString(GraphRef.ChatTTSModule, "Chat");
  if (ChatFunction == nullptr || !PyCallable_Check(ChatFunction)) {
    spdlog::error(
        "[WASI-NN] ChatTTS backend: Can not find Chat class in ChatTTS."sv);
    Py_XDECREF(GraphRef.ChatTTSModule);
    Env.NNGraph.pop_back();
    return WASINN::ErrNo::RuntimeError;
  }
  GraphRef.Chat = PyObject_CallObject(ChatFunction, nullptr);
  Py_XDECREF(ChatFunction);
  if (GraphRef.Chat == nullptr) {
    spdlog::error("[WASI-NN] ChatTTS backend: Can not create chat."sv);
    Py_XDECREF(GraphRef.ChatTTSModule);
    Env.NNGraph.pop_back();
    return WASINN::ErrNo::RuntimeError;
  }
  PyObject *LoadMethod = PyObject_GetAttrString(GraphRef.Chat, "load");
  if (LoadMethod == nullptr || !PyCallable_Check(LoadMethod)) {
    spdlog::error("[WASI-NN] ChatTTS backend: Can not load chat."sv);
    PyErr_Print();
    Env.NNGraph.pop_back();
    return WASINN::ErrNo::RuntimeError;
  }
  PyObject *Value = PyObject_CallObject(LoadMethod, nullptr);
  Py_XDECREF(Value);
  Py_DECREF(LoadMethod);
  // Store the loaded graph.
  GraphId = Env.NNGraph.size() - 1;

  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> initExecCtx(WasiNNEnvironment &Env, uint32_t GraphId,
                                  uint32_t &ContextId) noexcept {
  if (!Py_IsInitialized()) {
    spdlog::error(
        "[WASI-NN] ChatTTS backend: Model has been realse, please reload it."sv);
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
        "[WASI-NN] ChatTTS backend: Model has been realse, please reload it."sv);
    return WASINN::ErrNo::RuntimeError;
  }
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN] ChatTTS backend: setInput"sv);
  }

  // Set the input.
  std::string Prompt(reinterpret_cast<char *>(Tensor.Tensor.data()),
                     Tensor.Tensor.size());
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
    spdlog::info("[WASI-NN] ChatTTS backend: getOutput"sv);
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
        "[WASI-NN] ChatTTS backend: Model has been realse, please reload it."sv);
    return WASINN::ErrNo::RuntimeError;
  }
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN] ChatTTS backend: compute"sv);
  }
  if (CxtRef.Inputs.size() == 0) {
    spdlog::error("[WASI-NN] ChatTTS backend: Input is not set!"sv);
    return ErrNo::InvalidArgument;
  }
  PyObject *InputStr = PyUnicode_FromString(CxtRef.Inputs.c_str());
  PyObject *InferMethod = PyObject_GetAttrString(GraphRef.Chat, "infer");
  PyObject *Result = nullptr;
  if (InferMethod == nullptr || !PyCallable_Check(InferMethod)) {
    spdlog::error(
        "[WASI-NN] ChatTTS backend: Can not find infer method in Chat."sv);
    PyErr_Print();
    Py_XDECREF(InferMethod);
    return WASINN::ErrNo::RuntimeError;
  }
  PyObject *Args = PyTuple_Pack(1, InputStr);
  Result = PyObject_CallObject(InferMethod, Args);
  Py_XDECREF(Args);
  if (Result != nullptr) {
    PyObject *Wav0 = PyList_GetItem(Result, 0);
    PyObject *BytesObj = PyObject_CallMethod(Wav0, "tobytes", nullptr);
    char *Bytes = PyBytes_AsString(BytesObj);
    Py_ssize_t size = PyBytes_Size(BytesObj);
    CxtRef.Outputs = std::vector<uint8_t>(Bytes, Bytes + size);
    Py_DECREF(BytesObj);
    Py_DECREF(Wav0);
  } else {
    spdlog::error(
        "[WASI-NN] ChatTTS backend: Can not get output from infer method."sv);
    PyErr_Print();
    Py_XDECREF(Result);
    return WASINN::ErrNo::RuntimeError;
  }
  Py_XDECREF(Result);
  Py_XDECREF(InputStr);
  Py_XDECREF(InferMethod);
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
    Py_XDECREF(GraphRef.Chat);
    Py_XDECREF(GraphRef.ChatTTSModule);
    Py_Finalize();
  }
  return WASINN::ErrNo::Success;
}

#else
namespace {
Expect<WASINN::ErrNo> reportBackendNotSupported() noexcept {
  spdlog::error("[WASI-NN] ChatTTS backend is not built. use "
                "-WASMEDGE_PLUGIN_WASI_NN_BACKEND=\"ChatTTS\" to build it."sv);
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

} // namespace WasmEdge::Host::WASINN::ChatTTS