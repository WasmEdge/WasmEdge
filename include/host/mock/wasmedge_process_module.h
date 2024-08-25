// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "host/mock/wasmedge_process_func.h"
#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {

class WasmEdgeProcessModuleMock : public Runtime::Instance::ModuleInstance {
public:
  WasmEdgeProcessModuleMock()
      : Runtime::Instance::ModuleInstance("wasmedge_process") {
    addHostFunc("wasmedge_process_set_prog_name",
                std::make_unique<WasmEdgeProcessMock::SetProgName>());
    addHostFunc("wasmedge_process_add_arg",
                std::make_unique<WasmEdgeProcessMock::AddArg>());
    addHostFunc("wasmedge_process_add_env",
                std::make_unique<WasmEdgeProcessMock::AddEnv>());
    addHostFunc("wasmedge_process_add_stdin",
                std::make_unique<WasmEdgeProcessMock::AddStdIn>());
    addHostFunc("wasmedge_process_set_timeout",
                std::make_unique<WasmEdgeProcessMock::SetTimeOut>());
    addHostFunc("wasmedge_process_run",
                std::make_unique<WasmEdgeProcessMock::Run>());
    addHostFunc("wasmedge_process_get_exit_code",
                std::make_unique<WasmEdgeProcessMock::GetExitCode>());
    addHostFunc("wasmedge_process_get_stdout_len",
                std::make_unique<WasmEdgeProcessMock::GetStdOutLen>());
    addHostFunc("wasmedge_process_get_stdout",
                std::make_unique<WasmEdgeProcessMock::GetStdOut>());
    addHostFunc("wasmedge_process_get_stderr_len",
                std::make_unique<WasmEdgeProcessMock::GetStdErrLen>());
    addHostFunc("wasmedge_process_get_stderr",
                std::make_unique<WasmEdgeProcessMock::GetStdErr>());
  }
};

} // namespace Host
} // namespace WasmEdge
