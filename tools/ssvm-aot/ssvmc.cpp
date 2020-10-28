// SPDX-License-Identifier: Apache-2.0
#include "aot/compiler.h"
#include "common/filesystem.h"
#include "common/proposal.h"
#include "loader/loader.h"
#include "po/argument_parser.h"
#include "validator/validator.h"
#include <iostream>

int main(int Argc, const char *Argv[]) {
  namespace PO = SSVM::PO;
  using namespace std::literals;

  std::ios::sync_with_stdio(false);
  SSVM::Log::setErrorLoggingLevel();

  PO::Option<std::string> WasmName(PO::Description("Wasm file"s),
                                   PO::MetaVar("WASM"s));
  PO::Option<std::string> SoName(PO::Description("Wasm so file"s),
                                 PO::MetaVar("WASM_SO"s));

  PO::Option<PO::Toggle> DumpIR(
      PO::Description("Dump LLVM IR to `wasm.ll` and `wasm-opt.ll`."));

  PO::Option<PO::Toggle> InstructionCounting(PO::Description(
      "Generate code for counting Wasm instructions executed."));

  PO::Option<PO::Toggle> GasMeasuring(PO::Description(
      "Generate code for counting gas burned during execution."));

  PO::Option<PO::Toggle> BulkMemoryOperations(
      PO::Description("Enable Bulk-memory operations"));
  PO::Option<PO::Toggle> ReferenceTypes(
      PO::Description("Enable Reference types (externref)"));
  PO::Option<PO::Toggle> SIMD(PO::Description("Enable SIMD"));
  PO::Option<PO::Toggle> All(PO::Description("Enable all features"));

  if (!PO::ArgumentParser()
           .add_option(WasmName)
           .add_option(SoName)
           .add_option("dump", DumpIR)
           .add_option("ic", InstructionCounting)
           .add_option("gas", GasMeasuring)
           .add_option("enable-bulk-memory", BulkMemoryOperations)
           .add_option("enable-reference-types", ReferenceTypes)
           .add_option("enable-simd", SIMD)
           .add_option("enable-all", All)
           .parse(Argc, Argv)) {
    return 0;
  }

  SSVM::ProposalConfigure ProposalConf;
  if (BulkMemoryOperations.value()) {
    ProposalConf.addProposal(SSVM::Proposal::BulkMemoryOperations);
  }
  if (ReferenceTypes.value()) {
    ProposalConf.addProposal(SSVM::Proposal::ReferenceTypes);
  }
  if (SIMD.value()) {
    ProposalConf.addProposal(SSVM::Proposal::SIMD);
  }
  if (All.value()) {
    ProposalConf.addProposal(SSVM::Proposal::BulkMemoryOperations);
    ProposalConf.addProposal(SSVM::Proposal::ReferenceTypes);
    ProposalConf.addProposal(SSVM::Proposal::SIMD);
  }

  std::string InputPath = std::filesystem::absolute(WasmName.value()).string();
  std::string OutputPath = std::filesystem::absolute(SoName.value()).string();
  SSVM::Loader::Loader Loader(ProposalConf);

  std::vector<SSVM::Byte> Data;
  if (auto Res = Loader.loadFile(InputPath)) {
    Data = std::move(*Res);
  } else {
    const auto Err = static_cast<uint32_t>(Res.error());
    std::cout << "Load failed. Error code:" << Err << std::endl;
    return EXIT_FAILURE;
  }

  std::unique_ptr<SSVM::AST::Module> Module;
  if (auto Res = Loader.parseModule(Data)) {
    Module = std::move(*Res);
  } else {
    const auto Err = static_cast<uint32_t>(Res.error());
    std::cout << "Load failed. Error code:" << Err << std::endl;
    return EXIT_FAILURE;
  }

  {
    SSVM::Validator::Validator ValidatorEngine(ProposalConf);
    if (auto Res = ValidatorEngine.validate(*Module); !Res) {
      const auto Err = static_cast<uint32_t>(Res.error());
      std::cout << "Validate failed. Error code:" << Err << std::endl;
      return EXIT_FAILURE;
    }
  }

  {
    SSVM::AOT::Compiler Compiler;
    if (DumpIR.value()) {
      Compiler.setDumpIR();
    }
    if (InstructionCounting.value()) {
      Compiler.setInstructionCounting();
    }
    if (GasMeasuring.value()) {
      Compiler.setGasMeasuring();
    }
    if (auto Res = Compiler.compile(Data, *Module, OutputPath); !Res) {
      const auto Err = static_cast<uint32_t>(Res.error());
      std::cout << "Compile failed. Error code:" << Err << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
