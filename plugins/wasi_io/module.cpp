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
  addHostFunc("[method]error.to-debug-string",
              std::make_unique<ToDebugString>(Env));
}

WasiIOStreamsModule::WasiIOStreamsModule()
    : ComponentInstance("wasi:io/streams@0.2.0") {
  addHostType("input-stream", InputStream());

  addHostType("output-stream", OutputStream());
  addHostFunc("[method]output-stream.check-write",
              std::make_unique<OutputStream_CheckWrite>(Env));
  addHostFunc("[method]output-stream.write",
              std::make_unique<OutputStream_Write>(Env));
  addHostFunc("[method]output-stream.blocking-flush",
              std::make_unique<OutputStream_BlockingFlush>(Env));
  addHostFunc("[method]output-stream.blocking-write-and-flush",
              std::make_unique<OutputStream_BlockingWriteAndFlush>(Env));

  addHostType("error", StreamError::ast());
}

} // namespace Host
} // namespace WasmEdge
