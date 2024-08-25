// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "plugin/plugin.h"

#include "tensorflow/lite/c/c_api.h"

#include <cstdint>
#include <unordered_set>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeTensorflowLite {

enum class ErrNo : uint32_t {
  Success = 0,         // No error occurred.
  InvalidArgument = 1, // Caller module passed an invalid argument.
  InvalidEncoding = 2, // Invalid encoding.
  MissingMemory = 3,   // Caller module is missing a memory export.
  Busy = 4,            // Device or resource busy.
  RuntimeError = 5,    // Runtime Error.
};

struct Context {
  Context() = default;
  ~Context() { reset(); }
  void reset() noexcept {
    if (Interp) {
      TfLiteInterpreterDelete(Interp);
    }
    Interp = nullptr;
  }
  TfLiteInterpreter *Interp = nullptr;
};

struct TFLiteEnv {
  TFLiteEnv() noexcept { TFLiteContext.reserve(16U); }

  Context *getContext(const uint32_t ID) noexcept {
    auto It = RecycledIdx.find(ID);
    if (ID < TFLiteContext.size() && It == RecycledIdx.end()) {
      return &TFLiteContext[ID];
    }
    return nullptr;
  }
  uint32_t newContext() noexcept {
    uint32_t NewIdx = TFLiteContext.size();
    if (RecycledIdx.empty()) {
      TFLiteContext.emplace_back();
    } else {
      NewIdx = *RecycledIdx.begin();
      RecycledIdx.erase(NewIdx);
    }
    return NewIdx;
  }
  void deleteContext(const uint32_t ID) noexcept {
    if (ID < TFLiteContext.size()) {
      TFLiteContext[ID].reset();
      RecycledIdx.insert(ID);
    }
  }

private:
  std::unordered_set<uint32_t> RecycledIdx;
  std::vector<Context> TFLiteContext;
};

} // namespace WasmEdgeTensorflowLite
} // namespace Host
} // namespace WasmEdge
