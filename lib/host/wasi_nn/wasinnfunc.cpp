// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_nn/wasinnfunc.h"
#include "common/errcode.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/memory.h"

#ifdef WASMEDGE_WASINN_BUILD_OPENVINO
#include <c_api/ie_c_api.h>
#include <string>
#endif
namespace WasmEdge {
namespace Host {

const std::string map_target_to_string(uint32_t Target) {
  switch (Target) {
  case 0:
    return "CPU";
  case 1:
    return "GPU";
  case 2:
    return "TPU";
  default:
    return "";
  }
}

Expect<uint32_t> WasiNNLoad::body(Runtime::Instance::MemoryInstance *MemInst
                                  [[maybe_unused]],
                                  uint32_t BuilderPtr [[maybe_unused]],
                                  uint32_t BuilderLen [[maybe_unused]],
                                  uint32_t Encoding [[maybe_unused]],
                                  uint32_t Target [[maybe_unused]],
                                  uint32_t GraphPtr [[maybe_unused]]) {
  // GraphBuilders' Layout: |builder-0|builder-0 len|builder-1|builder-1 len|...
  // Check memory instance from module.
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::ExecutionFailed);
  }
  spdlog::info("here");
  uint32_t *GraphBuilders;
  uint32_t *Graph;
  GraphBuilders = MemInst->getPointer<uint32_t *>(BuilderPtr, 1);
  Graph = MemInst->getPointer<uint32_t *>(GraphPtr, 1);
  spdlog::info("got encoding {}, and OpenVINO flag {}", Encoding,
               Ctx.BackendsMapping.at("OpenVINO"));
  if (Encoding == Ctx.BackendsMapping.at("OpenVINO")) {
#ifdef WASMEDGE_WASINN_BUILD_OPENVINO
    spdlog::info("here");
    if (BuilderLen != 2) {
      spdlog::error("Wrong GraphBuilder Length {:d}, expecting 2", BuilderLen);
      return -1;
    }
    // IEStatusCode status;
    // if (Ctx.openvino_core == nullptr) {
    //   spdlog::info("Initialize OpenVINO Core");
    //   status = ie_core_create("", &Ctx.openvino_core);
    //   if (status != OK) {
    //     spdlog::error("Error happened when init OpenVINO core with status
    //     {}",
    //                   status);
    //     return -1;
    //   }
    // }
    spdlog::error("OpenVINO backend is not implemented.");
#else
    spdlog::error(
        "OpenVINO backend is not built. define -DWASINN_BUILD_OPENVINO "
        "to build it.");
#endif
  } else {
    spdlog::error("Current backend is not supported.");
  }
  spdlog::info("here");
  return -1;
}

Expect<uint32_t> WasiNNInitExecCtx::body(
    Runtime::Instance::MemoryInstance *MemInst [[maybe_unused]],
    uint32_t Graph [[maybe_unused]], uint32_t ContextPtr [[maybe_unused]]) {
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::ExecutionFailed);
  }

  if (Ctx.GraphBackends.find(Graph) == Ctx.GraphBackends.end()) {
    spdlog::error("init_execution_context: Graph does not exist");
    return -1;
  }
  [[maybe_unused]] uint32_t *Context =
      MemInst->getPointer<uint32_t *>(ContextPtr, 1);

  if (Ctx.GraphBackends[Graph] == Ctx.BackendsMapping.at("OpenVINO")) {
#ifdef WASMEDGE_WASINN_BUILD_OPENVINO
    return -1;
#else
    spdlog::error(
        "OpenVINO backend is not built. define -DWASINN_BUILD_OPENVINO "
        "to build it.");
#endif
  } else {
    spdlog::error("Current backend is not supported.");
  }
  return -1;
}

Expect<uint32_t> WasiNNSetInput::body(Runtime::Instance::MemoryInstance *MemInst
                                      [[maybe_unused]],
                                      uint32_t Context [[maybe_unused]],
                                      uint32_t Index [[maybe_unused]],
                                      uint32_t TensorPtr [[maybe_unused]]) {
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::ExecutionFailed);
  }

  if (Ctx.GraphContextBackends.find(Context) ==
      Ctx.GraphContextBackends.end()) {
    spdlog::error("set_input: Execution Context does not exist");
    return -1;
  }
  if (Ctx.GraphContextBackends[Context] == Ctx.BackendsMapping.at("OpenVINO")) {
#ifdef WASMEDGE_WASINN_BUILD_OPENVINO
    // reading tensor
    return -1;
#else
    spdlog::error(
        "OpenVINO backend is not built. define -DWASINN_BUILD_OPENVINO "
        "to build it.");
#endif
  } else {
    spdlog::error("Current backend is not supported.");
  }
  return -1;
}

Expect<uint32_t> WasiNNGetOuput::body(
    Runtime::Instance::MemoryInstance *MemInst [[maybe_unused]],
    uint32_t Context [[maybe_unused]], uint32_t Index [[maybe_unused]],
    uint32_t OutBuffer [[maybe_unused]],
    uint32_t OutBufferMaxSize [[maybe_unused]],
    uint32_t BytesWrittenPtr [[maybe_unused]]) {
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::ExecutionFailed);
  }

  [[maybe_unused]] uint8_t *OutBufferPtr;
  [[maybe_unused]] uint32_t *BytesWritten;

  if (Ctx.GraphContextBackends.find(Context) ==
      Ctx.GraphContextBackends.end()) {
    spdlog::error("get_output: Execution Context does not exist");
    return -1;
  }

  if (Ctx.GraphContextBackends[Context] == Ctx.BackendsMapping.at("OpenVINO")) {
#ifdef WASMEDGE_WASINN_BUILD_OPENVINO
    return -1;
#else
    spdlog::error(
        "OpenVINO backend is not built. define -DWASINN_BUILD_OPENVINO "
        "to build it.");
#endif
  } else {
    spdlog::error("Current backend is not supported.");
  }
  return -1;
}

Expect<uint32_t> WasiNNCompute::body(Runtime::Instance::MemoryInstance *MemInst
                                     [[maybe_unused]],
                                     uint32_t Context [[maybe_unused]]) {

  if (MemInst == nullptr) {
    return Unexpect(ErrCode::ExecutionFailed);
  }

  if (Ctx.GraphContextBackends.find(Context) ==
      Ctx.GraphContextBackends.end()) {
    spdlog::error("compute: Execution Context does not exist");
    return -1;
  }

  if (Ctx.GraphContextBackends[Context] == Ctx.BackendsMapping.at("OpenVINO")) {
#ifdef WASMEDGE_WASINN_BUILD_OPENVINO
    return -1;
#else
    spdlog::error(
        "OpenVINO backend is not built. define -DWASINN_BUILD_OPENVINO "
        "to build it.");
#endif
  } else {
    spdlog::error("Current backend is not supported.");
  }

  return -1;
}

} // namespace Host
} // namespace WasmEdge
