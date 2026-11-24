// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "common/span.h"
#include "common/spdlog.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace WasmEdge::Host::WASINN {

enum class ErrNo : uint32_t {
  Success = 0,              // No error occurred.
  InvalidArgument = 1,      // Caller module passed an invalid argument.
  InvalidEncoding = 2,      // Invalid encoding.
  MissingMemory = 3,        // Caller module is missing a memory export.
  Busy = 4,                 // Device or resource busy.
  RuntimeError = 5,         // Runtime Error.
  UnsupportedOperation = 6, // Unsupported Operation.
  TooLarge = 7,             // Too Large.
  NotFound = 8,             // Not Found.
  EndOfSequence = 100,      // End of Sequence Found.
  ContextFull = 101,        // Context Full.
  PromptTooLong = 102,      // Prompt Too Long.
  ModelNotFound = 103,      // Model Not Found.
};

enum class TensorType : uint8_t {
  F16 = 0,
  F32 = 1,
  F64 = 2,
  U8 = 3,
  I32 = 4,
  I64 = 5
};

enum class Device : uint32_t { CPU = 0, GPU = 1, TPU = 2, AUTO = 3 };

enum class Backend : uint8_t {
  OpenVINO = 0,
  ONNX = 1,
  Tensorflow = 2,
  PyTorch = 3,
  TensorflowLite = 4,
  Autodetect = 5,
  GGML = 6,
  NeuralSpeed = 7,
  Whisper = 9,
  MLX = 10,
  Piper = 11,
  ChatTTS = 12,
  OpenVINOGenAI = 13,
  BitNet = 14,
};

#define FOR_EACH_BACKEND(F)                                                    \
  F(OpenVINO)                                                                  \
  F(ONNX)                                                                      \
  F(Tensorflow)                                                                \
  F(PyTorch)                                                                   \
  F(TensorflowLite)                                                            \
  F(GGML)                                                                      \
  F(NeuralSpeed)                                                               \
  F(Whisper)                                                                   \
  F(Piper)                                                                     \
  F(ChatTTS)                                                                   \
  F(MLX)                                                                       \
  F(OpenVINOGenAI)                                                             \
  F(BitNet)

struct TensorData {
  Span<uint32_t> Dimension;
  WASINN::TensorType RType;
  Span<uint8_t> Tensor;
};

// The size in bytes of one element of a tensor type.
constexpr std::size_t tensorElementSize(TensorType Type) noexcept {
  switch (Type) {
  case TensorType::U8:
    return 1;
  case TensorType::F16:
    return 2;
  case TensorType::F32:
  case TensorType::I32:
    return 4;
  case TensorType::F64:
  case TensorType::I64:
    return 8;
  }
  return 0;
}

// Whether a dense tensor with the dimensions and the element size fills
// exactly ByteLength bytes. A tensor without dimensions is a scalar.
inline bool isDenseTensorSize(Span<const uint32_t> Dimension,
                              std::size_t ElementSize,
                              std::size_t ByteLength) noexcept {
  if (ElementSize == 0 || ByteLength % ElementSize != 0) {
    return false;
  }
  if (std::find(Dimension.begin(), Dimension.end(), 0U) != Dimension.end()) {
    return ByteLength == 0;
  }
  const std::size_t ExpectedCount = ByteLength / ElementSize;
  std::size_t Count = 1;
  for (const uint32_t Dim : Dimension) {
    if (Count > ExpectedCount / Dim) {
      return false;
    }
    Count *= Dim;
  }
  return Count == ExpectedCount;
}

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
    case WasmEdge::Host::WASINN::TensorType::F64:
      Name = "F64"sv;
      break;
    case WasmEdge::Host::WASINN::TensorType::U8:
      Name = "U8"sv;
      break;
    case WasmEdge::Host::WASINN::TensorType::I32:
      Name = "I32"sv;
      break;
    case WasmEdge::Host::WASINN::TensorType::I64:
      Name = "I64"sv;
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
