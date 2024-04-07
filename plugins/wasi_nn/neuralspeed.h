#pragma once

#include "plugin/plugin.h"
#include "types.h"
#include <mutex>
namespace WasmEdge::Host::WASINN {
struct WasiNNEnvironment;
}

namespace WasmEdge::Host::WASINN::NeuralSpeed {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_NEURAL_SPEED
#include <Python.h>
struct Graph {
  bool EnableDebugLog = true;
  static std::mutex py_mutex;
  inline static int GraphNumber = 0;
  Graph() noexcept {
    py_mutex.lock();
    if (GraphNumber == 0) {
      Py_Initialize();
    }
    GraphNumber++;
    py_mutex.unlock();
  }
  ~Graph() noexcept {
    Py_XDECREF(Model);
    Py_XDECREF(ModelClass);
    Py_XDECREF(NeuralSpeedModule);
    py_mutex.lock();
    if (GraphNumber == 1) {
      Py_Finalize();
    }
    GraphNumber--;
    py_mutex.unlock();
  }

  PyObject *Model;
  PyObject *NeuralSpeedModule;
  PyObject *ModelClass;
};
struct Context {
  Context(size_t Gid, Graph &) noexcept : GraphId(Gid) {}
  size_t GraphId;
  std::vector<long long int> Inputs;
  std::vector<long long int> Outputs;
};
#else
struct Graph {};
struct Context {
  Context(size_t, Graph &) noexcept {}
};
#endif



struct Environ {};

Expect<WASINN::ErrNo> load(WASINN::WasiNNEnvironment &Env,
                           Span<const Span<uint8_t>> Builders,
                           WASINN::Device Device, uint32_t &GraphId) noexcept;
Expect<WASINN::ErrNo> initExecCtx(WASINN::WasiNNEnvironment &Env,
                                  uint32_t GraphId,
                                  uint32_t &ContextId) noexcept;
Expect<WASINN::ErrNo> setInput(WASINN::WasiNNEnvironment &Env,
                               uint32_t ContextId, uint32_t Index,
                               const TensorData &Tensor) noexcept;
Expect<WASINN::ErrNo> getOutput(WASINN::WasiNNEnvironment &Env,
                                uint32_t ContextId, uint32_t Index,
                                Span<uint8_t> OutBuffer,
                                uint32_t &BytesWritten) noexcept;
Expect<WASINN::ErrNo> compute(WASINN::WasiNNEnvironment &Env,
                              uint32_t ContextId) noexcept;

} // namespace WasmEdge::Host::WASINN::NeuralSpeed