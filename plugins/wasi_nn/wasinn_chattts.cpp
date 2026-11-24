// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "wasinn_chattts.h"
#include "wasinnenv.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_CHATTTS
#include "simdjson.h"

#if !defined(_WIN32) && !defined(_WIN64) && !defined(__WIN32__) &&             \
    !defined(__TOS_WIN__) && !defined(__WINDOWS__)
#include <dlfcn.h>
#endif
#include <chrono>
#include <initializer_list>
#include <optional>
#include <string_view>
#include <time.h>
#include <utility>
#endif

namespace WasmEdge::Host::WASINN::ChatTTS {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_CHATTTS
#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) ||                \
    defined(__TOS_WIN__) || defined(__WINDOWS__)
HINSTANCE SharedLib = LoadLibrary(PYTHON_LIB_PATH);
#else
void *SharedLib = dlopen(PYTHON_LIB_PATH, RTLD_GLOBAL | RTLD_NOW);
#endif

namespace {
// An owned reference to a Python object. Use it only with the GIL held.
class PyRef {
public:
  PyRef() noexcept = default;
  explicit PyRef(PyObject *Object) noexcept : Object(Object) {}
  PyRef(PyRef &&Other) noexcept : Object(Other.release()) {}
  PyRef &operator=(PyRef &&Other) noexcept {
    Py_XDECREF(Object);
    Object = Other.release();
    return *this;
  }
  PyRef(const PyRef &) = delete;
  PyRef &operator=(const PyRef &) = delete;
  ~PyRef() noexcept { Py_XDECREF(Object); }
  PyObject *get() const noexcept { return Object; }
  PyObject *release() noexcept { return std::exchange(Object, nullptr); }
  explicit operator bool() const noexcept { return Object != nullptr; }

private:
  PyObject *Object = nullptr;
};

// Logs and clears the pending Python exception. PyErr_Print is not used
// because it exits the process when the exception is SystemExit.
void logPythonError() noexcept {
  PyObject *Type = nullptr;
  PyObject *Value = nullptr;
  PyObject *Traceback = nullptr;
  PyErr_Fetch(&Type, &Value, &Traceback);
  if (Type == nullptr) {
    return;
  }
  PyErr_NormalizeException(&Type, &Value, &Traceback);
  PyRef TypeRef(Type);
  PyRef ValueRef(Value);
  PyRef TracebackRef(Traceback);
  PyRef Text(ValueRef ? PyObject_Str(ValueRef.get()) : nullptr);
  const char *Message = Text ? PyUnicode_AsUTF8(Text.get()) : nullptr;
  spdlog::error("[WASI-NN] ChatTTS backend: Python error {}: {}"sv,
                ValueRef ? Py_TYPE(ValueRef.get())->tp_name : "unknown",
                Message != nullptr ? Message : "");
  PyErr_Clear();
}

bool setItem(const PyRef &Dict, const char *Key, const PyRef &Value) noexcept {
  return Value && PyDict_SetItemString(Dict.get(), Key, Value.get()) == 0;
}

PyRef createParams(PyObject *Chat, const char *Name,
                   const PyRef &Kwargs) noexcept {
  PyRef ParamsClass(PyObject_GetAttrString(Chat, Name));
  if (!ParamsClass) {
    return {};
  }
  PyRef Args(PyTuple_New(0));
  if (!Args) {
    return {};
  }
  return PyRef(PyObject_Call(ParamsClass.get(), Args.get(), Kwargs.get()));
}

template <typename T>
WASINN::ErrNo getOptionalOption(simdjson::dom::object &Object,
                                std::string_view Key,
                                std::optional<T> &Result) noexcept {
  T Value{};
  if (auto Error = Object[Key].get(Value)) {
    if (Error == simdjson::error_code::NO_SUCH_FIELD) {
      return WASINN::ErrNo::Success;
    }
    spdlog::error(
        "[WASI-NN] ChatTTS backend: Unable to retrieve the \"{}\" option: {}"sv,
        Key, simdjson::error_message(Error));
    return WASINN::ErrNo::InvalidArgument;
  }
  Result = Value;
  return WASINN::ErrNo::Success;
}
} // namespace

Expect<WASINN::ErrNo> load(WASINN::WasiNNEnvironment &, WASINN::Graph &G,
                           Span<const Span<uint8_t>>, WASINN::Device) noexcept {
  auto &GraphRef = G.get<Graph>();
  // Initialize the plugin parameters.
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN] ChatTTS backend: Load."sv);
  }

  // Create Model class
  if (!Py_IsInitialized()) {
    Py_Initialize();
    if (PyGILState_Check()) {
      PyEval_SaveThread();
    }
  }
  GIL Lock;
  if (GraphRef.ChatTTSModule == nullptr) {
    GraphRef.ChatTTSModule = PyImport_ImportModule("ChatTTS");
    if (GraphRef.ChatTTSModule == nullptr) {
      spdlog::error(
          "[WASI-NN] ChatTTS backend: Can not find ChatTTS library."sv);
      logPythonError();
      return WASINN::ErrNo::RuntimeError;
    }
  }
  if (GraphRef.Chat == nullptr) {
    PyObject *ChatFunction =
        PyObject_GetAttrString(GraphRef.ChatTTSModule, "Chat");
    if (ChatFunction == nullptr || !PyCallable_Check(ChatFunction)) {
      spdlog::error(
          "[WASI-NN] ChatTTS backend: Can not find Chat class in ChatTTS."sv);
      logPythonError();
      return WASINN::ErrNo::RuntimeError;
    }
    GraphRef.Chat = PyObject_CallObject(ChatFunction, nullptr);
    Py_XDECREF(ChatFunction);
    if (GraphRef.Chat == nullptr) {
      spdlog::error("[WASI-NN] ChatTTS backend: Can not create chat."sv);
      logPythonError();
      return WASINN::ErrNo::RuntimeError;
    }
    PyObject *LoadMethod = PyObject_GetAttrString(GraphRef.Chat, "load");
    if (LoadMethod == nullptr || !PyCallable_Check(LoadMethod)) {
      spdlog::error("[WASI-NN] ChatTTS backend: Can not load chat."sv);
      logPythonError();
      return WASINN::ErrNo::RuntimeError;
    }
    PyObject *Value = PyObject_CallObject(LoadMethod, nullptr);
    Py_XDECREF(Value);
    Py_XDECREF(LoadMethod);
  }

  return WASINN::ErrNo::Success;
}

Expect<WASINN::ErrNo> initExecCtx(WasiNNEnvironment &, WASINN::Graph &,
                                  WASINN::Context &) noexcept {
  if (!Py_IsInitialized()) {
    spdlog::error(
        "[WASI-NN] ChatTTS backend: Model has been released, please reload it."sv);
    return WASINN::ErrNo::RuntimeError;
  }
  return ErrNo::Success;
}

Expect<WASINN::ErrNo> setInput(WasiNNEnvironment &, WASINN::Graph &G,
                               WASINN::Context &C, uint32_t Index,
                               const TensorData &Tensor) noexcept {
  auto &CxtRef = C.get<Context>();
  auto &GraphRef = G.get<Graph>();
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
      return ErrNo::InvalidEncoding;
    }
    simdjson::dom::object Object;
    if (Doc.get(Object)) {
      spdlog::error(
          "[WASI-NN] ChatTTS backend: The metadata is not a JSON object."sv);
      return ErrNo::InvalidArgument;
    }
    std::optional<std::string_view> Prompt;
    std::optional<double> Temperature;
    std::optional<double> TopK;
    std::optional<double> TopP;
    std::optional<std::string_view> SpkEmb;
    for (auto Err : {getOptionalOption(Object, "prompt"sv, Prompt),
                     getOptionalOption(Object, "temperature"sv, Temperature),
                     getOptionalOption(Object, "top_K"sv, TopK),
                     getOptionalOption(Object, "top_P"sv, TopP),
                     getOptionalOption(Object, "spk_emb"sv, SpkEmb)}) {
      if (Err != ErrNo::Success) {
        return Err;
      }
    }

    GIL Lock;
    auto ReportError = []() noexcept {
      spdlog::error(
          "[WASI-NN] ChatTTS backend: Unable to apply the metadata."sv);
      logPythonError();
      return WASINN::ErrNo::RuntimeError;
    };
    PyRef NewParamsRefineText;
    if (Prompt) {
      PyRef Kwargs(PyDict_New());
      if (!Kwargs || !setItem(Kwargs, "prompt",
                              PyRef(PyUnicode_FromStringAndSize(
                                  Prompt->data(),
                                  static_cast<Py_ssize_t>(Prompt->size()))))) {
        return ReportError();
      }
      NewParamsRefineText =
          createParams(GraphRef.Chat, "RefineTextParams", Kwargs);
      if (!NewParamsRefineText) {
        return ReportError();
      }
    }
    PyRef NewParamsInferCode;
    if (Temperature || TopK || TopP || SpkEmb) {
      PyRef Kwargs(PyDict_New());
      if (!Kwargs) {
        return ReportError();
      }
      if (Temperature && !setItem(Kwargs, "temperature",
                                  PyRef(PyFloat_FromDouble(*Temperature)))) {
        return ReportError();
      }
      if (TopK && !setItem(Kwargs, "top_K", PyRef(PyFloat_FromDouble(*TopK)))) {
        return ReportError();
      }
      if (TopP && !setItem(Kwargs, "top_P", PyRef(PyFloat_FromDouble(*TopP)))) {
        return ReportError();
      }
      if (SpkEmb) {
        PyRef Spk(
            *SpkEmb == "random"sv
                ? PyObject_CallMethod(GraphRef.Chat, "sample_random_speaker",
                                      nullptr)
                : PyUnicode_FromStringAndSize(
                      SpkEmb->data(), static_cast<Py_ssize_t>(SpkEmb->size())));
        if (!setItem(Kwargs, "spk_emb", Spk)) {
          return ReportError();
        }
      }
      NewParamsInferCode =
          createParams(GraphRef.Chat, "InferCodeParams", Kwargs);
      if (!NewParamsInferCode) {
        return ReportError();
      }
    }
    if (NewParamsRefineText) {
      Py_XDECREF(GraphRef.ParamsRefineText);
      GraphRef.ParamsRefineText = NewParamsRefineText.release();
    }
    if (NewParamsInferCode) {
      Py_XDECREF(GraphRef.ParamsInferCode);
      GraphRef.ParamsInferCode = NewParamsInferCode.release();
    }
    return WASINN::ErrNo::Success;
  }
  return WASINN::ErrNo::InvalidArgument;
}

Expect<WASINN::ErrNo> getOutput(WasiNNEnvironment &, WASINN::Graph &G,
                                WASINN::Context &C, uint32_t Index,
                                Span<uint8_t> OutBuffer,
                                uint32_t &BytesWritten) noexcept {
  auto &CxtRef = C.get<Context>();
  auto &GraphRef = G.get<Graph>();
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN] ChatTTS backend: getOutput"sv);
  }
  if (Index == 0) {
    BytesWritten = static_cast<uint32_t>(CxtRef.Outputs.size());
    if (OutBuffer.size() < CxtRef.Outputs.size()) {
      spdlog::error("[WASI-NN] ChatTTS backend: output buffer too small, "
                    "need {} bytes but got {}."sv,
                    CxtRef.Outputs.size(), OutBuffer.size());
      return WASINN::ErrNo::TooLarge;
    }
    std::copy_n(CxtRef.Outputs.data(), CxtRef.Outputs.size(), OutBuffer.data());
    return WASINN::ErrNo::Success;
  }
  return WASINN::ErrNo::InvalidArgument;
}

Expect<WASINN::ErrNo> compute(WasiNNEnvironment &, WASINN::Graph &G,
                              WASINN::Context &C) noexcept {
  if (!Py_IsInitialized()) {
    spdlog::error(
        "[WASI-NN] ChatTTS backend: Model has been released, please reload it."sv);
    return WASINN::ErrNo::RuntimeError;
  }
  auto &CxtRef = C.get<Context>();
  auto &GraphRef = G.get<Graph>();
  if (GraphRef.EnableDebugLog) {
    spdlog::info("[WASI-NN] ChatTTS backend: compute"sv);
  }
  if (CxtRef.Inputs.size() == 0) {
    spdlog::error("[WASI-NN] ChatTTS backend: Input is not set!"sv);
    return ErrNo::InvalidArgument;
  }
  GIL Lock;
  PyObject *InputStr = PyUnicode_FromString(CxtRef.Inputs.c_str());
  PyObject *InferMethod = PyObject_GetAttrString(GraphRef.Chat, "infer");
  PyObject *Result = nullptr;
  if (InferMethod == nullptr || !PyCallable_Check(InferMethod)) {
    spdlog::error(
        "[WASI-NN] ChatTTS backend: Can not find infer method in Chat."sv);
    logPythonError();
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
    PyObject *Index = PyLong_FromLong(0);
    PyObject *Wav0 = PyObject_GetItem(Result, Index);
    Py_XDECREF(Index);
    PyObject *BytesObj = PyObject_CallMethod(Wav0, "tobytes", nullptr);
    Py_XDECREF(Wav0);
    char *Bytes = PyBytes_AsString(BytesObj);
    Py_ssize_t size = PyBytes_Size(BytesObj);
    CxtRef.Outputs = std::vector<uint8_t>(Bytes, Bytes + size);
    Py_XDECREF(BytesObj);
  } else {
    spdlog::error(
        "[WASI-NN] ChatTTS backend: Can not get output from infer method."sv);
    logPythonError();
    Py_XDECREF(InputStr);
    Py_XDECREF(InferMethod);
    return WASINN::ErrNo::RuntimeError;
  }
  Py_XDECREF(Result);
  Py_XDECREF(InputStr);
  Py_XDECREF(InferMethod);
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

Expect<WASINN::ErrNo> load(WASINN::WasiNNEnvironment &, WASINN::Graph &,
                           Span<const Span<uint8_t>>, WASINN::Device) noexcept {
  return reportBackendNotSupported();
}
Expect<WASINN::ErrNo> initExecCtx(WASINN::WasiNNEnvironment &, WASINN::Graph &,
                                  WASINN::Context &) noexcept {
  return reportBackendNotSupported();
}
Expect<WASINN::ErrNo> setInput(WASINN::WasiNNEnvironment &, WASINN::Graph &,
                               WASINN::Context &, uint32_t,
                               const TensorData &) noexcept {
  return reportBackendNotSupported();
}
Expect<WASINN::ErrNo> getOutput(WASINN::WasiNNEnvironment &, WASINN::Graph &,
                                WASINN::Context &, uint32_t, Span<uint8_t>,
                                uint32_t &) noexcept {
  return reportBackendNotSupported();
}
Expect<WASINN::ErrNo> compute(WASINN::WasiNNEnvironment &, WASINN::Graph &,
                              WASINN::Context &) noexcept {
  return reportBackendNotSupported();
}
#endif

} // namespace WasmEdge::Host::WASINN::ChatTTS
