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

  PO::Option<PO::Toggle> ConfGenericBinary(
      PO::Description("Generate a generic binary"sv));

  PO::Option<PO::Toggle> ConfDumpIR(
      PO::Description("Dump LLVM IR to `wasm.ll` and `wasm-opt.ll`."sv));

  PO::Option<PO::Toggle> ConfInstructionCounting(PO::Description(
      "Generate code for counting Wasm instructions executed."sv));

  PO::Option<PO::Toggle> ConfGasMeasuring(PO::Description(
      "Generate code for counting gas burned during execution."sv));

  PO::Option<PO::Toggle> PropMutGlobals(
      PO::Description("Disable Import/Export of mutable globals proposal"sv));
  PO::Option<PO::Toggle> PropNonTrapF2IConvs(PO::Description(
      "Disable Non-trapping float-to-int conversions proposal"sv));
  PO::Option<PO::Toggle> PropSignExtendOps(
      PO::Description("Disable Sign-extension operators proposal"sv));
  PO::Option<PO::Toggle> PropMultiValue(
      PO::Description("Disable Multi-value proposal"sv));
  PO::Option<PO::Toggle> PropBulkMemOps(
      PO::Description("Disable Bulk memory operations proposal"sv));
  PO::Option<PO::Toggle> PropRefTypes(
      PO::Description("Disable Reference types proposal"sv));
  PO::Option<PO::Toggle> PropSIMD(PO::Description("Enable SIMD proposal"sv));
  PO::Option<PO::Toggle> PropAll(PO::Description("Enable all features"sv));

  auto Parser = PO::ArgumentParser();
  if (!Parser.add_option(WasmName)
           .add_option(SoName)
           .add_option("dump"sv, ConfDumpIR)
           .add_option("ic"sv, ConfInstructionCounting)
           .add_option("gas"sv, ConfGasMeasuring)
           .add_option("generic-binary"sv, ConfGenericBinary)
           .add_option("disable-import-export-mut-globals"sv, PropMutGlobals)
           .add_option("disable-non-trap-float-to-int"sv, PropNonTrapF2IConvs)
           .add_option("disable-sign-extension-operators"sv, PropSignExtendOps)
           .add_option("disable-multi-value"sv, PropMultiValue)
           .add_option("disable-bulk-memory"sv, PropBulkMemOps)
           .add_option("disable-reference-types"sv, PropRefTypes)
           .add_option("enable-simd"sv, PropSIMD)
           .add_option("enable-all"sv, PropAll)
           .parse(Argc, Argv)) {
    return EXIT_FAILURE;
  }
  if (Parser.isVersion()) {
    std::cout << Argv[0] << " version "sv << WasmEdge::kVersionString << '\n';
    return EXIT_SUCCESS;
  }

  WasmEdge::Configure Conf;
  if (PropMutGlobals.value()) {
    Conf.removeProposal(WasmEdge::Proposal::ImportExportMutGlobals);
  }
  if (PropNonTrapF2IConvs.value()) {
    Conf.removeProposal(WasmEdge::Proposal::NonTrapFloatToIntConversions);
  }
  if (PropSignExtendOps.value()) {
    Conf.removeProposal(WasmEdge::Proposal::SignExtensionOperators);
  }
  if (PropMultiValue.value()) {
    Conf.removeProposal(WasmEdge::Proposal::MultiValue);
  }
  if (PropBulkMemOps.value()) {
    Conf.removeProposal(WasmEdge::Proposal::BulkMemoryOperations);
  }
  if (PropRefTypes.value()) {
    Conf.removeProposal(WasmEdge::Proposal::ReferenceTypes);
  }
  if (PropSIMD.value()) {
    Conf.addProposal(WasmEdge::Proposal::SIMD);
  }
  if (PropAll.value()) {
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
    if (ConfDumpIR.value()) {
      Conf.getCompilerConfigure().setDumpIR(true);
    }
    if (ConfInstructionCounting.value()) {
      Conf.getCompilerConfigure().setInstructionCounting(true);
    }
    if (ConfGasMeasuring.value()) {
      Conf.getCompilerConfigure().setCostMeasuring(true);
    }
    if (ConfGenericBinary.value()) {
      Conf.getCompilerConfigure().setGenericBinary(true);
    }
    WasmEdge::AOT::Compiler Compiler(Conf);
    if (auto Res = Compiler.compile(Data, *Module, OutputPath); !Res) {
      const auto Err = static_cast<uint32_t>(Res.error());
      std::cout << "Compile failed. Error code:" << Err << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
