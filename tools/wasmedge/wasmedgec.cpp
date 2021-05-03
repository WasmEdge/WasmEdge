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
  namespace PO = WasmEdge::PO;
  using namespace std::literals;

  std::ios::sync_with_stdio(false);
  WasmEdge::Log::setErrorLoggingLevel();

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
      PO::Description("Disable Bulk-memory operations"sv));
  PO::Option<PO::Toggle> ReferenceTypes(
      PO::Description("Disable Reference types (externref)"sv));
  PO::Option<PO::Toggle> SIMD(PO::Description("Enable SIMD"sv));
  PO::Option<PO::Toggle> All(PO::Description("Enable all features"sv));

  auto Parser = PO::ArgumentParser();
  if (!Parser.add_option(WasmName)
           .add_option(SoName)
           .add_option("dump"sv, DumpIR)
           .add_option("ic"sv, InstructionCounting)
           .add_option("gas"sv, GasMeasuring)
           .add_option("disable-bulk-memory"sv, BulkMemoryOperations)
           .add_option("disable-reference-types"sv, ReferenceTypes)
           .add_option("enable-simd"sv, SIMD)
           .add_option("enable-all"sv, All)
           .parse(Argc, Argv)) {
    return EXIT_FAILURE;
  }
  if (Parser.isVersion()) {
    std::cout << Argv[0] << " version "sv << WasmEdge::kVersionString << '\n';
    return EXIT_SUCCESS;
  }

  WasmEdge::Configure Conf;
  if (BulkMemoryOperations.value()) {
    Conf.removeProposal(WasmEdge::Proposal::BulkMemoryOperations);
  }
  if (ReferenceTypes.value()) {
    Conf.removeProposal(WasmEdge::Proposal::ReferenceTypes);
  }
  if (SIMD.value()) {
    Conf.addProposal(WasmEdge::Proposal::SIMD);
  }
  if (All.value()) {
    Conf.addProposal(WasmEdge::Proposal::BulkMemoryOperations);
    Conf.addProposal(WasmEdge::Proposal::ReferenceTypes);
    Conf.addProposal(WasmEdge::Proposal::SIMD);
  }

  std::filesystem::path InputPath = std::filesystem::absolute(WasmName.value());
  std::filesystem::path OutputPath = std::filesystem::absolute(SoName.value());
  WasmEdge::Loader::Loader Loader(Conf);

  std::vector<WasmEdge::Byte> Data;
  if (auto Res = Loader.loadFile(InputPath)) {
    Data = std::move(*Res);
  } else {
    const auto Err = static_cast<uint32_t>(Res.error());
    std::cout << "Load failed. Error code:" << Err << std::endl;
    return EXIT_FAILURE;
  }

  std::unique_ptr<WasmEdge::AST::Module> Module;
  if (auto Res = Loader.parseModule(Data)) {
    Module = std::move(*Res);
  } else {
    const auto Err = static_cast<uint32_t>(Res.error());
    std::cout << "Load failed. Error code:" << Err << std::endl;
    return EXIT_FAILURE;
  }

  {
    WasmEdge::Validator::Validator ValidatorEngine(Conf);
    if (auto Res = ValidatorEngine.validate(*Module); !Res) {
      const auto Err = static_cast<uint32_t>(Res.error());
      std::cout << "Validate failed. Error code:" << Err << std::endl;
      return EXIT_FAILURE;
    }
  }

  {
    WasmEdge::AOT::Compiler Compiler;
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
