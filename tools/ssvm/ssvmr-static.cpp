// SPDX-License-Identifier: Apache-2.0
#include "common/filesystem.h"
#include "common/proposal.h"
#include "common/value.h"
#include "host/wasi/wasimodule.h"
#include "vm/configure.h"
#include "vm/vm.h"

#include <cstdlib>
#include <iostream>

int main(int Argc, const char *Argv[], const char *Env[]) {
  using namespace std::literals;

  std::ios::sync_with_stdio(false);
  SSVM::Log::setErrorLoggingLevel();

  SSVM::ProposalConfigure ProposalConf;
  ProposalConf.addProposal(SSVM::Proposal::BulkMemoryOperations);
  ProposalConf.addProposal(SSVM::Proposal::ReferenceTypes);
  ProposalConf.addProposal(SSVM::Proposal::SIMD);

  /// check Argv[0] is the wasm file
  if (std::filesystem::u8path(Argv[0]).extension() != ".wasm") {
    ++Argv;
    --Argc;
  }
  const auto InputPath =
      std::filesystem::absolute(std::filesystem::u8path(Argv[0]));
  SSVM::VM::Configure Conf;
  Conf.addVMType(SSVM::VM::Configure::VMType::Wasi);
  Conf.addVMType(SSVM::VM::Configure::VMType::SSVM_Process);
  SSVM::VM::VM VM(ProposalConf, Conf);

  SSVM::Host::WasiModule *WasiMod = dynamic_cast<SSVM::Host::WasiModule *>(
      VM.getImportModule(SSVM::VM::Configure::VMType::Wasi));

  int Envc = 0;
  for (const char **EnvP = Env; *EnvP != nullptr; ++EnvP) {
    ++Envc;
  }
  WasiMod->getEnv().init(std::array{"/:/"s}, InputPath,
                         std::vector<std::string>(Argv + 1, Argv + Argc),
                         std::vector<std::string>(Env, Env + Envc));

  // command mode
  if (auto Result = VM.runWasmFile(InputPath.u8string(), "_start")) {
    return WasiMod->getEnv().getExitCode();
  } else {
    return EXIT_FAILURE;
  }
}
