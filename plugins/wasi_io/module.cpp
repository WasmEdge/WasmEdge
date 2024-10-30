// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "module.h"
#include "func.h"

#include <memory>

namespace WasmEdge {
namespace Host {

WasiIOErrorModule::WasiIOErrorModule()
    : ComponentInstance("wasi:io/error@0.2.0") {
  addHostType("error", IoError(getEnv()));
  addHostFunc("[resource-drop]error", std::make_unique<DropError>(Env));
}

WasiIOStreamsModule::WasiIOStreamsModule()
    : ComponentInstance("wasi:io/streams@0.2.0") {
  addHostType("input-stream", InputStream());
  addHostFunc("[resource-drop]input-stream",
              std::make_unique<DropInputStream>(Env));

  addHostType("output-stream", OutputStream());
  addHostFunc("[method]output-stream.check-write",
              std::make_unique<OutputStream_CheckWrite>(Env));
  addHostFunc("[method]output-stream.write",
              std::make_unique<OutputStream_Write>(Env));
  addHostFunc("[method]output-stream.blocking-flush",
              std::make_unique<OutputStream_BlockingFlush>(Env));
  addHostFunc("[method]output-stream.blocking-write-and-flush",
              std::make_unique<OutputStream_BlockingWriteAndFlush>(Env));
  addHostFunc("[resource-drop]output-stream",
              std::make_unique<DropOutputStream>(Env));

  addHostType("error", StreamError());
  addHostFunc("[resource-drop]error", std::make_unique<DropStreamError>(Env));
}

} // namespace Host
} // namespace WasmEdge
