// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once
#include "common/log.h"
#include "common/span.h"
#include <cstdint>

namespace WasmEdge::Host::WASINN {

enum class ErrNo : uint32_t {
  Success = 0,         // No error occurred.
  InvalidArgument = 1, // Caller module passed an invalid argument.
  InvalidEncoding = 2, // Invalid encoding.
  MissingMemory = 3,   // Caller module is missing a memory export.
  Busy = 4,            // Device or resource busy.
  RuntimeError = 5,    // Runtime Error.
};

enum class TensorType : uint8_t { F16 = 0, F32 = 1, U8 = 2, I32 = 3 };

enum class Device : uint32_t { CPU = 0, GPU = 1, TPU = 2 };

enum class Backend : uint8_t {
  OpenVINO = 0,
  ONNX = 1,
  Tensorflow = 2,
  PyTorch = 3,
  TensorflowLite = 4
};

#define FOR_EACH_BACKEND(F)                                                    \
  F(OpenVINO) F(ONNX) F(Tensorflow) F(PyTorch) F(TensorflowLite)

struct TensorData {
  Span<uint32_t> Dimension;
  WASINN::TensorType RType;
  Span<uint8_t> Tensor;
};

} // namespace WasmEdge::Host::WASINN

template <>
struct fmt::formatter<WasmEdge::Host::WASINN::TensorType>
    : fmt::formatter<std::string_view> {
  fmt::format_context::iterator format(WasmEdge::Host::WASINN::TensorType RType,
                                       fmt::format_context &Ctx) const {
    using namespace std::literals;
    std::string_view Name;
    switch (RType) {
    case WasmEdge::Host::WASINN::TensorType::F16:
      Name = "F16"sv;
      break;
    case WasmEdge::Host::WASINN::TensorType::F32:
      Name = "F32"sv;
      break;
    case WasmEdge::Host::WASINN::TensorType::U8:
      Name = "U8"sv;
      break;
    case WasmEdge::Host::WASINN::TensorType::I32:
      Name = "I32"sv;
      break;
    default:
      Name = "Unknown"sv;
    }
    return fmt::formatter<std::string_view>::format(Name, Ctx);
  }
};

template <>
struct fmt::formatter<WasmEdge::Host::WASINN::Device>
    : fmt::formatter<std::string_view> {
  fmt::format_context::iterator format(WasmEdge::Host::WASINN::Device Target,
                                       fmt::format_context &Ctx) const {
    using namespace std::literals;
    std::string_view Name;
    switch (Target) {
    case WasmEdge::Host::WASINN::Device::CPU:
      Name = "CPU"sv;
      break;
    case WasmEdge::Host::WASINN::Device::GPU:
      Name = "GPU"sv;
      break;
    case WasmEdge::Host::WASINN::Device::TPU:
      Name = "TPU"sv;
      break;
    default:
      Name = "Unknown"sv;
    }
    return fmt::formatter<std::string_view>::format(Name, Ctx);
  }
};
