// SPDX-License-Identifier: Apache-2.0
#include "aot/compiler.h"
#include "common/configure.h"
#include "common/filesystem.h"
#include "common/version.h"
#include "loader/loader.h"
#include "po/argument_parser.h"
#include "validator/validator.h"
#include <iostream>

int main(int Argc, const char *Argv[]) {
  namespace PO = SSVM::PO;
  using namespace std::literals;

  std::ios::sync_with_stdio(false);
  SSVM::Log::setErrorLoggingLevel();

  PO::Option<std::string> WasmName(PO::Description("Wasm file"sv),
                                   PO::MetaVar("WASM"sv));
  PO::Option<std::string> SoName(PO::Description("Wasm so file"sv),
                                 PO::MetaVar("WASM_SO"sv));

  PO::Option<PO::Toggle> DumpIR(
      PO::Description("Dump LLVM IR to `wasm.ll` and `wasm-opt.ll`."sv));

  PO::Option<PO::Toggle> InstructionCounting(PO::Description(
      "Generate code for counting Wasm instructions executed."sv));

  PO::Option<PO::Toggle> GasMeasuring(PO::Description(
      "Generate code for counting gas burned during execution."sv));

  PO::Option<PO::Toggle> BulkMemoryOperations(
      PO::Description("Enable Bulk-memory operations"sv));
  PO::Option<PO::Toggle> ReferenceTypes(
      PO::Description("Enable Reference types (externref)"sv));
  PO::Option<PO::Toggle> SIMD(PO::Description("Enable SIMD"sv));
  PO::Option<PO::Toggle> All(PO::Description("Enable all features"sv));

  auto Parser = PO::ArgumentParser();
  if (!Parser.add_option(WasmName)
           .add_option(SoName)
           .add_option("dump"sv, DumpIR)
           .add_option("ic"sv, InstructionCounting)
           .add_option("gas"sv, GasMeasuring)
           .add_option("enable-bulk-memory"sv, BulkMemoryOperations)
           .add_option("enable-reference-types"sv, ReferenceTypes)
           .add_option("enable-simd"sv, SIMD)
           .add_option("enable-all"sv, All)
           .parse(Argc, Argv)) {
    return EXIT_FAILURE;
  }
  if (Parser.isVersion()) {
    std::cout << Argv[0] << " version "sv << SSVM::kVersionString << '\n';
    return EXIT_SUCCESS;
  }

  SSVM::Configure Conf;
  if (BulkMemoryOperations.value()) {
    Conf.addProposal(SSVM::Proposal::BulkMemoryOperations);
  }
  if (ReferenceTypes.value()) {
    Conf.addProposal(SSVM::Proposal::ReferenceTypes);
  }
  if (SIMD.value()) {
    Conf.addProposal(SSVM::Proposal::SIMD);
  }
  if (All.value()) {
    Conf.addProposal(SSVM::Proposal::BulkMemoryOperations);
    Conf.addProposal(SSVM::Proposal::ReferenceTypes);
    Conf.addProposal(SSVM::Proposal::SIMD);
  }

  std::filesystem::path InputPath = std::filesystem::absolute(WasmName.value());
  std::filesystem::path OutputPath = std::filesystem::absolute(SoName.value());
  SSVM::Loader::Loader Loader(Conf);

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
    SSVM::Validator::Validator ValidatorEngine(Conf);
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
