// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN] ChatTTS backend: Load."sv);
  }

  // Create Model class
  if (!Py_IsInitialized()) {
    Py_Initialize();
  }
  if (GraphRef.ChatTTSModule == nullptr) {
    GraphRef.ChatTTSModule = PyImport_ImportModule("ChatTTS");
    if (GraphRef.ChatTTSModule == nullptr) {
      spdlog::error(
          "[WASI-NN] ChatTTS backend: Can not find ChatTTS library."sv);
      Env.NNGraph.pop_back();
      return WASINN::ErrNo::RuntimeError;
    }
  }
  if (GraphRef.Chat == nullptr) {
    PyObject *ChatFunction =
        PyObject_GetAttrString(GraphRef.ChatTTSModule, "Chat");
    if (ChatFunction == nullptr || !PyCallable_Check(ChatFunction)) {
      spdlog::error(
          "[WASI-NN] ChatTTS backend: Can not find Chat class in ChatTTS."sv);
      Env.NNGraph.pop_back();
      return WASINN::ErrNo::RuntimeError;
    }
    GraphRef.Chat = PyObject_CallObject(ChatFunction, nullptr);
    Py_XDECREF(ChatFunction);
    if (GraphRef.Chat == nullptr) {
      spdlog::error("[WASI-NN] ChatTTS backend: Can not create chat."sv);
      Env.NNGraph.pop_back();
      return WASINN::ErrNo::RuntimeError;
    }
    PyObject *LoadMethod = PyObject_GetAttrString(GraphRef.Chat, "load");
    if (LoadMethod == nullptr || !PyCallable_Check(LoadMethod)) {
      spdlog::error("[WASI-NN] ChatTTS backend: Can not load chat."sv);
      Env.NNGraph.pop_back();
      return WASINN::ErrNo::RuntimeError;
    }
    PyObject *Value = PyObject_CallObject(LoadMethod, nullptr);
    Py_XDECREF(Value);
    Py_XDECREF(LoadMethod);
  }
  // Store the loaded graph.
  GraphId = Env.NNGraph.size() - 1;

  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> initExecCtx(WasiNNEnvironment &Env, uint32_t GraphId,
                                  uint32_t &ContextId) noexcept {
  if (!Py_IsInitialized()) {
    spdlog::error(
        "[WASI-NN] ChatTTS backend: Model has been released, please reload it."sv);
    return WASINN::ErrNo::RuntimeError;
  }
  Env.NNContext.emplace_back(GraphId, Env.NNGraph[GraphId]);
  ContextId = Env.NNContext.size() - 1;
  return ErrNo::Success;
}

Expect<WASINN::ErrNo> setInput(WasiNNEnvironment &Env, uint32_t ContextId,
                               uint32_t Index,
                               const TensorData &Tensor) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  if (!Py_IsInitialized()) {
    spdlog::error(
        "[WASI-NN] ChatTTS backend: Model has been released, please reload it."sv);
    return WASINN::ErrNo::RuntimeError;
  }
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN] ChatTTS backend: setInput"sv);
  }
  if (Index == 0) {
    // Set the input.
    std::string Prompt(reinterpret_cast<char *>(Tensor.Tensor.data()),
                       Tensor.Tensor.size());
    CxtRef.Inputs.clear();
    CxtRef.Inputs = Prompt;
    return WASINN::ErrNo::Success;
  } else if (Index == 1) {
    // Set metadata.
    std::string Metadata = std::string(
        reinterpret_cast<char *>(Tensor.Tensor.data()), Tensor.Tensor.size());
    simdjson::dom::parser Parser;
    simdjson::dom::element Doc;
    auto ParseError = Parser.parse(Metadata).get(Doc);
    if (ParseError) {
      spdlog::error("[WASI-NN] ChatTTS backend: Parse metadata error"sv);
      Env.NNGraph.pop_back();
      return ErrNo::InvalidEncoding;
    }
    // Handle Refine Text Params
    PyObject *PromptObj = nullptr;
    if (Doc.at_key("prompt").error() == simdjson::SUCCESS) {
      std::string_view PromptView;
      auto Err = Doc["prompt"].get<std::string_view>().get(PromptView);
      if (Err) {
        spdlog::error(
            "[WASI-NN] ChatTTS backend: Unable to retrieve the prompt option."sv);
        return ErrNo::InvalidArgument;
      }
      PromptObj = PyUnicode_FromString(std::string(PromptView).c_str());
    }
    if (PromptObj != nullptr) {
      PyObject *Args = PyTuple_New(0);
      PyObject *Kwargs = PyDict_New();
      PyDict_SetItemString(Kwargs, "prompt", PromptObj);
      PyObject *RefineTextParamsFun =
          PyObject_GetAttrString(GraphRef.Chat, "RefineTextParams");
      GraphRef.ParamsRefineText =
          PyObject_Call(RefineTextParamsFun, Args, Kwargs);
      Py_XDECREF(PromptObj);
      Py_XDECREF(Args);
      Py_XDECREF(Kwargs);
      Py_XDECREF(RefineTextParamsFun);
    }
    // Handle Infer Code Params
    PyObject *InferKwargs = PyDict_New();
    if (Doc.at_key("temperature").error() == simdjson::SUCCESS) {
      double Temperature;
      auto Err = Doc["temperature"].get<double>().get(Temperature);
      if (Err) {
        spdlog::error(
            "[WASI-NN] ChatTTS backend: Unable to retrieve the temperature option."sv);
        return ErrNo::InvalidArgument;
      }
      PyObject *TemperatureObject = PyFloat_FromDouble(Temperature);
      PyDict_SetItemString(InferKwargs, "temperature", TemperatureObject);
      Py_XDECREF(TemperatureObject);
    }
    if (Doc.at_key("top_K").error() == simdjson::SUCCESS) {
      double TopK;
      auto Err = Doc["top_K"].get<double>().get(TopK);
      if (Err) {
        spdlog::error(
            "[WASI-NN] ChatTTS backend: Unable to retrieve the topK option."sv);
        return ErrNo::InvalidArgument;
      }
      PyObject *TopKObject = PyFloat_FromDouble(TopK);
      PyDict_SetItemString(InferKwargs, "top_K", TopKObject);
      Py_XDECREF(TopKObject);
    }
    if (Doc.at_key("top_P").error() == simdjson::SUCCESS) {
      double TopP;
      auto Err = Doc["top_P"].get<double>().get(TopP);
      if (Err) {
        spdlog::error(
            "[WASI-NN] ChatTTS backend: Unable to retrieve the temperature option."sv);
        return ErrNo::InvalidArgument;
      }
      PyObject *TopPObject = PyFloat_FromDouble(TopP);
      PyDict_SetItemString(InferKwargs, "top_P", TopPObject);
      Py_XDECREF(TopPObject);
    }
    if (Doc.at_key("spk_emb").error() == simdjson::SUCCESS) {
      std::string_view SpkEmb;
      auto Err = Doc["spk_emb"].get<std::string_view>().get(SpkEmb);
      if (Err) {
        spdlog::error(
            "[WASI-NN] ChatTTS backend: Unable to retrieve the spk_emb option."sv);
        return ErrNo::InvalidArgument;
      }
      if (SpkEmb == "random") {
        PyObject *SampleRandomSpeaker =
            PyObject_GetAttrString(GraphRef.Chat, "sample_random_speaker");
        PyObject *Spk = PyObject_CallNoArgs(SampleRandomSpeaker);
        PyDict_SetItemString(InferKwargs, "spk_emb", Spk);
        Py_XDECREF(SampleRandomSpeaker);
        Py_XDECREF(Spk);
      } else {
        PyObject *Spk = PyUnicode_FromString(std::string(SpkEmb).c_str());
        PyDict_SetItemString(InferKwargs, "spk_emb", Spk);
        Py_XDECREF(Spk);
      }
    }
    if (PyDict_Size(InferKwargs) != 0) {
      PyObject *Args = PyTuple_New(0);
      PyObject *InferCodeParams =
          PyObject_GetAttrString(GraphRef.Chat, "InferCodeParams");
      GraphRef.ParamsInferCode =
          PyObject_Call(InferCodeParams, Args, InferKwargs);
      Py_XDECREF(Args);
      Py_XDECREF(InferCodeParams);
    }
    Py_XDECREF(InferKwargs);
    return WASINN::ErrNo::Success;
  }
  return WASINN::ErrNo::InvalidArgument;
}

Expect<WASINN::ErrNo> getOutput(WasiNNEnvironment &Env, uint32_t ContextId,
                                uint32_t Index, Span<uint8_t> OutBuffer,
                                uint32_t &BytesWritten) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN] ChatTTS backend: getOutput"sv);
  }
  if (Index == 0) {
    std::copy_n(CxtRef.Outputs.data(), CxtRef.Outputs.size(), OutBuffer.data());
    BytesWritten = CxtRef.Outputs.size();
    return WASINN::ErrNo::Success;
  }
  return WASINN::ErrNo::InvalidArgument;
}
Expect<WASINN::ErrNo> compute(WasiNNEnvironment &Env,
                              uint32_t ContextId) noexcept {
  if (!Py_IsInitialized()) {
    spdlog::error(
        "[WASI-NN] ChatTTS backend: Model has been released, please reload it."sv);
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
  if (GraphRef.ParamsRefineText == nullptr &&
      GraphRef.ParamsInferCode == nullptr) {
    PyObject *Args = PyTuple_Pack(1, InputStr);
    Result = PyObject_CallObject(InferMethod, Args);
    Py_XDECREF(Args);
  } else {
    PyObject *Args = PyTuple_New(0);
    PyObject *Kwargs = PyDict_New();
    PyDict_SetItemString(Kwargs, "text", InputStr);
    if (GraphRef.ParamsRefineText != nullptr) {
      PyDict_SetItemString(Kwargs, "params_refine_text",
                           GraphRef.ParamsRefineText);
    }
    if (GraphRef.ParamsInferCode != nullptr) {
      PyDict_SetItemString(Kwargs, "params_infer_code",
                           GraphRef.ParamsInferCode);
    }
    Result = PyObject_Call(InferMethod, Args, Kwargs);
    Py_XDECREF(Args);
    Py_XDECREF(Kwargs);
  }
  if (Result != nullptr) {
    PyObject *Wav0 = PyList_GetItem(Result, 0);
    PyObject *BytesObj = PyObject_CallMethod(Wav0, "tobytes", nullptr);
    char *Bytes = PyBytes_AsString(BytesObj);
    Py_ssize_t size = PyBytes_Size(BytesObj);
    CxtRef.Outputs = std::vector<uint8_t>(Bytes, Bytes + size);
    Py_XDECREF(BytesObj);
  } else {
    spdlog::error(
        "[WASI-NN] ChatTTS backend: Can not get output from infer method."sv);
    Py_XDECREF(InputStr);
    Py_XDECREF(InferMethod);
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
    Py_XDECREF(GraphRef.ParamsRefineText);
    Py_XDECREF(GraphRef.ParamsInferCode);
    Py_XDECREF(GraphRef.Chat);
    Py_XDECREF(GraphRef.ChatTTSModule);
    GraphRef.Chat = nullptr;
    GraphRef.ChatTTSModule = nullptr;
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
