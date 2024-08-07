// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "processmodule.h"
#include "processfunc.h"

#include <memory>

namespace WasmEdge {
namespace Host {

WasmEdgeProcessModule::WasmEdgeProcessModule()
    : ModuleInstance("wasmedge_process") {
  addHostFunc("wasmedge_process_set_prog_name",
              std::make_unique<WasmEdgeProcessSetProgName>(Env));
  addHostFunc("wasmedge_process_add_arg",
              std::make_unique<WasmEdgeProcessAddArg>(Env));
  addHostFunc("wasmedge_process_add_env",
              std::make_unique<WasmEdgeProcessAddEnv>(Env));
  addHostFunc("wasmedge_process_add_stdin",
              std::make_unique<WasmEdgeProcessAddStdIn>(Env));
  addHostFunc("wasmedge_process_set_timeout",
              std::make_unique<WasmEdgeProcessSetTimeOut>(Env));
  addHostFunc("wasmedge_process_run",
              std::make_unique<WasmEdgeProcessRun>(Env));
  addHostFunc("wasmedge_process_get_exit_code",
              std::make_unique<WasmEdgeProcessGetExitCode>(Env));
  addHostFunc("wasmedge_process_get_stdout_len",
              std::make_unique<WasmEdgeProcessGetStdOutLen>(Env));
  addHostFunc("wasmedge_process_get_stdout",
              std::make_unique<WasmEdgeProcessGetStdOut>(Env));
  addHostFunc("wasmedge_process_get_stderr_len",
              std::make_unique<WasmEdgeProcessGetStdErrLen>(Env));
  addHostFunc("wasmedge_process_get_stderr",
              std::make_unique<WasmEdgeProcessGetStdErr>(Env));
}

} // namespace Host
} // namespace WasmEdge
