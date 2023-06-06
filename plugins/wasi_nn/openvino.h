// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "plugin/plugin.h"
#include "types.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO
#include "common/log.h"
#include <c_api/ie_c_api.h>
#include <vector>

template <>
struct fmt::formatter<IEStatusCode> : fmt::formatter<std::string_view> {
  fmt::format_context::iterator format(IEStatusCode Code,
                                       fmt::format_context &Ctx) const {
    using namespace std::literals;
    std::string_view Name;
    switch (Code) {
    case OK:
      Name = "OK"sv;
      break;
    case GENERAL_ERROR:
      Name = "GENERAL_ERROR"sv;
      break;
    case NOT_IMPLEMENTED:
      Name = "NOT_IMPLEMENTED"sv;
      break;
    case NETWORK_NOT_LOADED:
      Name = "NETWORK_NOT_LOADED"sv;
      break;
    case PARAMETER_MISMATCH:
      Name = "PARAMETER_MISMATCH"sv;
      break;
    case NOT_FOUND:
      Name = "NOT_FOUND"sv;
      break;
    case OUT_OF_BOUNDS:
      Name = "OUT_OF_BOUNDS"sv;
      break;
    case UNEXPECTED:
      Name = "UNEXPECTED"sv;
      break;
    case REQUEST_BUSY:
      Name = "REQUEST_BUSY"sv;
      break;
    case RESULT_NOT_READY:
      Name = "RESULT_NOT_READY"sv;
      break;
    case NOT_ALLOCATED:
      Name = "NOT_ALLOCATED"sv;
      break;
    case INFER_NOT_STARTED:
      Name = "INFER_NOT_STARTED"sv;
      break;
    case NETWORK_NOT_READ:
      Name = "NETWORK_NOT_READ"sv;
      break;
    case INFER_CANCELLED:
      Name = "INFER_CANCELLED"sv;
      break;
    default:
      Name = "Unknown"sv;
    }
    return fmt::formatter<std::string_view>::format(Name, Ctx);
  }
};
#endif

namespace WasmEdge::Host::WASINN {
struct WasiNNEnvironment;
}

namespace WasmEdge::Host::WASINN::OpenVINO {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO
struct Graph {
  ~Graph() noexcept {
    if (OpenVINONetwork) {
      ie_network_free(&OpenVINONetwork);
    }
    if (OpenVINOExecNetwork) {
      ie_exec_network_free(&OpenVINOExecNetwork);
    }
    if (OpenVINOWeightBlob) {
      ie_blob_free(&OpenVINOWeightBlob);
    }
    for (auto &I : OpenVINOInputNames) {
      if (I) {
        ie_network_name_free(&I);
      }
    }
    for (auto &I : OpenVINOOutputNames) {
      if (I) {
        ie_network_name_free(&I);
      }
    }
  }
  ie_network_t *OpenVINONetwork = nullptr;
  ie_executable_network_t *OpenVINOExecNetwork = nullptr;
  ie_blob_t *OpenVINOWeightBlob = nullptr;
  std::vector<char *> OpenVINOInputNames;
  std::vector<char *> OpenVINOOutputNames;
};

struct Context {
  Context(size_t GId, Graph &G) noexcept : GraphId(GId) {
    IEStatusCode Status = ie_exec_network_create_infer_request(
        G.OpenVINOExecNetwork, &OpenVINOInferRequest);
    if (Status != IEStatusCode::OK) {
      OpenVINOInferRequest = nullptr;
      spdlog::error("[WASI-NN] Unable to create infer request for OpenVINO");
    }
  }
  ~Context() noexcept {
    if (OpenVINOInferRequest) {
      ie_infer_request_free(&OpenVINOInferRequest);
    }
  }
  size_t GraphId;
  ie_infer_request_t *OpenVINOInferRequest = nullptr;
};

struct Environ {
  Environ() noexcept {
    if (ie_core_create("", &OpenVINOCore) != IEStatusCode::OK) {
      spdlog::error(
          "[WASI-NN] Error happened when initializing OpenVINO core.");
    }
  }
  ~Environ() noexcept {
    if (OpenVINOCore) {
      ie_core_free(&OpenVINOCore);
    }
  }
  ie_core_t *OpenVINOCore = nullptr;
};
#else
struct Graph {};
struct Context {
  Context(size_t, Graph &) noexcept {}
};
struct Environ {};
#endif

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
} // namespace WasmEdge::Host::WASINN::OpenVINO
