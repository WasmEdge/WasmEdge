// SPDX-License-Identifier: Apache-2.0
#include "host/ssvm_process/processmodule.h"
#include "host/ssvm_process/processfunc.h"

#include <memory>

namespace SSVM {
namespace Host {

SSVMProcessModule::SSVMProcessModule() : ImportObject("ssvm_process") {
  addHostFunc("ssvm_process_set_prog_name",
              std::make_unique<SSVMProcessSetProgName>(Env));
  addHostFunc("ssvm_process_add_arg", std::make_unique<SSVMProcessAddArg>(Env));
  addHostFunc("ssvm_process_add_env", std::make_unique<SSVMProcessAddEnv>(Env));
  addHostFunc("ssvm_process_add_stdin",
              std::make_unique<SSVMProcessAddStdIn>(Env));
  addHostFunc("ssvm_process_set_timeout",
              std::make_unique<SSVMProcessSetTimeOut>(Env));
  addHostFunc("ssvm_process_run", std::make_unique<SSVMProcessRun>(Env));
  addHostFunc("ssvm_process_get_exit_code",
              std::make_unique<SSVMProcessGetExitCode>(Env));
  addHostFunc("ssvm_process_get_stdout_len",
              std::make_unique<SSVMProcessGetStdOutLen>(Env));
  addHostFunc("ssvm_process_get_stdout",
              std::make_unique<SSVMProcessGetStdOut>(Env));
  addHostFunc("ssvm_process_get_stderr_len",
              std::make_unique<SSVMProcessGetStdErrLen>(Env));
  addHostFunc("ssvm_process_get_stderr",
              std::make_unique<SSVMProcessGetStdErr>(Env));
}

} // namespace Host
} // namespace SSVM
