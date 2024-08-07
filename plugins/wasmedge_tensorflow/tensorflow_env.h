// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "plugin/plugin.h"

#include "tensorflow/c/c_api.h"

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeTensorflow {

enum class ErrNo : uint32_t {
  Success = 0,         // No error occurred.
  InvalidArgument = 1, // Caller module passed an invalid argument.
  InvalidEncoding = 2, // Invalid encoding.
  MissingMemory = 3,   // Caller module is missing a memory export.
  Busy = 4,            // Device or resource busy.
  RuntimeError = 5,    // Runtime Error.
};

struct TensorList {
  void reset() noexcept {
    for (uint32_t I = 0; I < DataList.size(); ++I) {
      if (DataList[I]) {
        TF_DeleteTensor(DataList[I]);
      }
    }
    NameMap.clear();
    OperList.clear();
    DataList.clear();
  }

  std::unordered_map<std::string, uint32_t> NameMap;
  std::vector<TF_Output> OperList;
  std::vector<TF_Tensor *> DataList;
};

struct Context {
  Context() noexcept { Stat = TF_NewStatus(); }
  ~Context() noexcept {
    reset();
    TF_DeleteStatus(Stat);
  }

  void clearInputs() noexcept { Inputs.reset(); }

  void clearOutputs() noexcept { Outputs.reset(); }

  void reset() noexcept {
    if (GraphOpts) {
      TF_DeleteImportGraphDefOptions(GraphOpts);
      GraphOpts = nullptr;
    }
    if (Buffer) {
      TF_DeleteBuffer(Buffer);
      Buffer = nullptr;
    }
    if (Graph) {
      TF_DeleteGraph(Graph);
      Graph = nullptr;
    }
    if (SessionOpts) {
      TF_DeleteSessionOptions(SessionOpts);
      SessionOpts = nullptr;
    }
    if (Session) {
      TF_CloseSession(Session, Stat);
      TF_DeleteSession(Session, Stat);
      Session = nullptr;
    }
    clearInputs();
    clearOutputs();
  }

  TF_Status *Stat;
  TF_ImportGraphDefOptions *GraphOpts = nullptr;
  TF_Buffer *Buffer = nullptr;
  TF_Graph *Graph = nullptr;
  TF_SessionOptions *SessionOpts = nullptr;
  TF_Session *Session = nullptr;
  struct TensorList Inputs;
  struct TensorList Outputs;
};

struct TFEnv {
  TFEnv() noexcept { TFContext.reserve(16U); }

  Context *getContext(const uint32_t ID) noexcept {
    auto It = RecycledIdx.find(ID);
    if (ID < TFContext.size() && It == RecycledIdx.end()) {
      return &TFContext[ID];
    }
    return nullptr;
  }
  uint32_t newContext() noexcept {
    uint32_t NewIdx = TFContext.size();
    if (RecycledIdx.empty()) {
      TFContext.emplace_back();
    } else {
      NewIdx = *RecycledIdx.begin();
      RecycledIdx.erase(NewIdx);
    }
    return NewIdx;
  }
  void deleteContext(const uint32_t ID) noexcept {
    if (ID < TFContext.size()) {
      TFContext[ID].reset();
      RecycledIdx.insert(ID);
    }
  }

private:
  std::unordered_set<uint32_t> RecycledIdx;
  std::vector<Context> TFContext;
};

} // namespace WasmEdgeTensorflow
} // namespace Host
} // namespace WasmEdge
