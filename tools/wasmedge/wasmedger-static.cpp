// SPDX-License-Identifier: Apache-2.0
#include "common/configure.h"
#include "common/filesystem.h"
#include "common/value.h"
#include "host/wasi/wasimodule.h"
#include "vm/vm.h"

#include <cstdlib>
#include <iostream>

int main(int Argc, const char *Argv[], const char *Env[]) {
  using namespace std::literals;

  std::ios::sync_with_stdio(false);
  WasmEdge::Log::setErrorLoggingLevel();

  /// check Argv[0] is the wasm file
  if (std::filesystem::u8path(Argv[0]).extension() != ".wasm") {
    ++Argv;
    --Argc;
  }
  const auto InputPath =
      std::filesystem::absolute(std::filesystem::u8path(Argv[0]));
  WasmEdge::Configure Conf;
  Conf.addHostRegistration(WasmEdge::HostRegistration::Wasi);
  Conf.addHostRegistration(WasmEdge::HostRegistration::WasmEdge_Process);
  Conf.addProposal(WasmEdge::Proposal::BulkMemoryOperations);
  Conf.addProposal(WasmEdge::Proposal::ReferenceTypes);
  Conf.addProposal(WasmEdge::Proposal::SIMD);
  WasmEdge::VM::VM VM(Conf);

  WasmEdge::Host::WasiModule *WasiMod =
      dynamic_cast<WasmEdge::Host::WasiModule *>(
          VM.getImportModule(WasmEdge::HostRegistration::Wasi));

  int Envc = 0;
  for (const char **EnvP = Env; *EnvP != nullptr; ++EnvP) {
    ++Envc;
  }
  WasiMod->getEnv().init(std::array{"/:/"s}, InputPath,
                         std::vector<std::string>(Argv + 1, Argv + Argc),
                         std::vector<std::string>(Env, Env + Envc));

  // command mode
  if (auto Result = VM.runWasmFile(InputPath.u8string(), "_start");
      Result || Result.error() == WasmEdge::ErrCode::Terminated) {
    return WasiMod->getEnv().getExitCode();
  } else {
    return EXIT_FAILURE;
  }
}
