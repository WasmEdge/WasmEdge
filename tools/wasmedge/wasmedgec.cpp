// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "aot/compiler.h"
#include "common/configure.h"
#include "common/filesystem.h"
#include "common/version.h"
#include "loader/loader.h"
#include "po/argument_parser.h"
#include "validator/validator.h"
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

int main(int Argc, const char *Argv[]) {
  namespace PO = WasmEdge::PO;
  using namespace std::literals;

  std::ios::sync_with_stdio(false);
  WasmEdge::Log::setInfoLoggingLevel();

  PO::Option<std::string> WasmName(PO::Description("Wasm file"sv),
                                   PO::MetaVar("WASM"sv));
  PO::Option<std::string> SoName(PO::Description("Wasm so file"sv),
                                 PO::MetaVar("WASM_SO"sv));

  PO::Option<PO::Toggle> ConfGenericBinary(
      PO::Description("Generate a generic binary"sv));

  PO::Option<PO::Toggle> ConfDumpIR(
      PO::Description("Dump LLVM IR to `wasm.ll` and `wasm-opt.ll`."sv));

  PO::Option<PO::Toggle> ConfInterruptible(
      PO::Description("Generate a interruptible binary"sv));

  PO::Option<PO::Toggle> ConfEnableInstructionCounting(PO::Description(
      "Enable generating code for counting Wasm instructions executed."sv));
  PO::Option<PO::Toggle> ConfEnableGasMeasuring(PO::Description(
      "Enable generating code for counting gas burned during execution."sv));
  PO::Option<PO::Toggle> ConfEnableTimeMeasuring(PO::Description(
      "Enable generating code for counting time during execution."sv));
  PO::Option<PO::Toggle> ConfEnableAllStatistics(PO::Description(
      "Enable generating code for all statistics options include instruction counting, gas measuring, and execution time"sv));

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
  PO::Option<PO::Toggle> PropSIMD(PO::Description("Disable SIMD proposal"sv));
  PO::Option<PO::Toggle> PropMultiMem(
      PO::Description("Enable Multiple memories proposal"sv));
  PO::Option<PO::Toggle> PropTailCall(
      PO::Description("Enable Tail-call proposal"sv));
  PO::Option<PO::Toggle> PropAll(PO::Description("Enable all features"sv));

  auto Parser = PO::ArgumentParser();
  if (!Parser.add_option(WasmName)
           .add_option(SoName)
           .add_option("dump"sv, ConfDumpIR)
           .add_option("interruptible"sv, ConfInterruptible)
           .add_option("enable-instruction-count"sv,
                       ConfEnableInstructionCounting)
           .add_option("enable-gas-measuring"sv, ConfEnableGasMeasuring)
           .add_option("enable-time-measuring"sv, ConfEnableTimeMeasuring)
           .add_option("enable-all-statistics"sv, ConfEnableAllStatistics)
           .add_option("generic-binary"sv, ConfGenericBinary)
           .add_option("disable-import-export-mut-globals"sv, PropMutGlobals)
           .add_option("disable-non-trap-float-to-int"sv, PropNonTrapF2IConvs)
           .add_option("disable-sign-extension-operators"sv, PropSignExtendOps)
           .add_option("disable-multi-value"sv, PropMultiValue)
           .add_option("disable-bulk-memory"sv, PropBulkMemOps)
           .add_option("disable-reference-types"sv, PropRefTypes)
           .add_option("disable-simd"sv, PropSIMD)
           .add_option("enable-multi-memory"sv, PropMultiMem)
           .add_option("enable-tail-call"sv, PropTailCall)
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
    Conf.removeProposal(WasmEdge::Proposal::SIMD);
  }
  if (PropMultiMem.value()) {
    Conf.addProposal(WasmEdge::Proposal::MultiMemories);
  }
  if (PropTailCall.value()) {
    Conf.addProposal(WasmEdge::Proposal::TailCall);
  }
  if (PropAll.value()) {
    Conf.addProposal(WasmEdge::Proposal::MultiMemories);
    Conf.addProposal(WasmEdge::Proposal::TailCall);
  }

  std::filesystem::path InputPath = std::filesystem::absolute(WasmName.value());
  std::filesystem::path OutputPath = std::filesystem::absolute(SoName.value());
  WasmEdge::Loader::Loader Loader(Conf);

  std::vector<WasmEdge::Byte> Data;
  if (auto Res = Loader.loadFile(InputPath)) {
    Data = std::move(*Res);
  } else {
    const auto Err = static_cast<uint32_t>(Res.error());
    spdlog::error("Load failed. Error code: {}", Err);
    return EXIT_FAILURE;
  }

  std::unique_ptr<WasmEdge::AST::Module> Module;
  if (auto Res = Loader.parseModule(Data)) {
    Module = std::move(*Res);
  } else {
    const auto Err = static_cast<uint32_t>(Res.error());
    spdlog::error("Parse Module failed. Error code: {}", Err);
    return EXIT_FAILURE;
  }

  {
    WasmEdge::Validator::Validator ValidatorEngine(Conf);
    if (auto Res = ValidatorEngine.validate(*Module); !Res) {
      const auto Err = static_cast<uint32_t>(Res.error());
      spdlog::error("Validate Module failed. Error code: {}", Err);
      return EXIT_FAILURE;
    }
  }

  {
    if (ConfDumpIR.value()) {
      Conf.getCompilerConfigure().setDumpIR(true);
    }
    if (ConfInterruptible.value()) {
      Conf.getCompilerConfigure().setInterruptible(true);
    }
    if (ConfEnableAllStatistics.value()) {
      Conf.getStatisticsConfigure().setInstructionCounting(true);
      Conf.getStatisticsConfigure().setCostMeasuring(true);
      Conf.getStatisticsConfigure().setTimeMeasuring(true);
    } else {
      if (ConfEnableInstructionCounting.value()) {
        Conf.getStatisticsConfigure().setInstructionCounting(true);
      }
      if (ConfEnableGasMeasuring.value()) {
        Conf.getStatisticsConfigure().setCostMeasuring(true);
      }
      if (ConfEnableTimeMeasuring.value()) {
        Conf.getStatisticsConfigure().setTimeMeasuring(true);
      }
    }
    if (ConfGenericBinary.value()) {
      Conf.getCompilerConfigure().setGenericBinary(true);
    }
    if (OutputPath.extension().u8string() == ".so"sv) {
      Conf.getCompilerConfigure().setOutputFormat(
          WasmEdge::CompilerConfigure::OutputFormat::Native);
    }
    WasmEdge::AOT::Compiler Compiler(Conf);
    if (auto Res = Compiler.compile(Data, *Module, OutputPath); !Res) {
      const auto Err = static_cast<uint32_t>(Res.error());
      spdlog::error("Compilation failed. Error code: {}", Err);
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
