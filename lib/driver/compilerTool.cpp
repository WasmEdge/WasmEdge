// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/configure.h"
#include "common/defines.h"
#include "common/filesystem.h"
#include "common/version.h"
#include "driver/compiler.h"
#include "loader/loader.h"
#include "validator/validator.h"
#include "llvm/codegen.h"
#include "llvm/compiler.h"
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Driver {

int Compiler([[maybe_unused]] struct DriverCompilerOptions &Opt) noexcept {
  using namespace std::literals;

  std::ios::sync_with_stdio(false);
  Log::setInfoLoggingLevel();

#ifdef WASMEDGE_USE_LLVM

  Configure Conf;
  if (Opt.PropMutGlobals.value()) {
    Conf.removeProposal(Proposal::ImportExportMutGlobals);
  }
  if (Opt.PropNonTrapF2IConvs.value()) {
    Conf.removeProposal(Proposal::NonTrapFloatToIntConversions);
  }
  if (Opt.PropSignExtendOps.value()) {
    Conf.removeProposal(Proposal::SignExtensionOperators);
  }
  if (Opt.PropMultiValue.value()) {
    Conf.removeProposal(Proposal::MultiValue);
  }
  if (Opt.PropBulkMemOps.value()) {
    Conf.removeProposal(Proposal::BulkMemoryOperations);
  }
  if (Opt.PropRefTypes.value()) {
    Conf.removeProposal(Proposal::ReferenceTypes);
  }
  if (Opt.PropSIMD.value()) {
    Conf.removeProposal(Proposal::SIMD);
  }
  if (Opt.PropRelaxedSIMD.value()) {
    Conf.addProposal(Proposal::RelaxSIMD);
  }
  if (Opt.PropMultiMem.value()) {
    Conf.addProposal(Proposal::MultiMemories);
  }
  if (Opt.PropTailCall.value()) {
    Conf.addProposal(Proposal::TailCall);
  }
  if (Opt.PropExtendConst.value()) {
    Conf.addProposal(Proposal::ExtendedConst);
  }
  if (Opt.PropThreads.value()) {
    Conf.addProposal(Proposal::Threads);
  }
  if (Opt.PropAll.value()) {
    Conf.addProposal(Proposal::MultiMemories);
    Conf.addProposal(Proposal::TailCall);
    Conf.addProposal(Proposal::ExtendedConst);
    Conf.addProposal(Proposal::Threads);
  }

  if (Opt.PropOptimizationLevel.value() == "0") {
    Conf.getCompilerConfigure().setOptimizationLevel(
        WasmEdge::CompilerConfigure::OptimizationLevel::O0);
  } else if (Opt.PropOptimizationLevel.value() == "1") {
    Conf.getCompilerConfigure().setOptimizationLevel(
        WasmEdge::CompilerConfigure::OptimizationLevel::O1);
  } else if (Opt.PropOptimizationLevel.value() == "3") {
    Conf.getCompilerConfigure().setOptimizationLevel(
        WasmEdge::CompilerConfigure::OptimizationLevel::O3);
  } else if (Opt.PropOptimizationLevel.value() == "s") {
    Conf.getCompilerConfigure().setOptimizationLevel(
        WasmEdge::CompilerConfigure::OptimizationLevel::Os);
  } else if (Opt.PropOptimizationLevel.value() == "z") {
    Conf.getCompilerConfigure().setOptimizationLevel(
        WasmEdge::CompilerConfigure::OptimizationLevel::Oz);
  } else {
    Conf.getCompilerConfigure().setOptimizationLevel(
        WasmEdge::CompilerConfigure::OptimizationLevel::O2);
  }

  // Set force interpreter here to load instructions of function body forcibly.
  Conf.getRuntimeConfigure().setForceInterpreter(true);

  std::filesystem::path InputPath =
      std::filesystem::absolute(std::filesystem::u8path(Opt.WasmName.value()));
  std::filesystem::path OutputPath =
      std::filesystem::absolute(std::filesystem::u8path(Opt.SoName.value()));
  Loader::Loader Loader(Conf);

  std::vector<Byte> Data;
  if (auto Res = Loader.loadFile(InputPath)) {
    Data = std::move(*Res);
  } else {
    const auto Err = static_cast<uint32_t>(Res.error());
    spdlog::error("Load failed. Error code: {}", Err);
    return EXIT_FAILURE;
  }

  std::unique_ptr<AST::Module> Module;
  if (auto Res = Loader.parseModule(Data)) {
    Module = std::move(*Res);
  } else {
    const auto Err = static_cast<uint32_t>(Res.error());
    spdlog::error("Parse Module failed. Error code: {}", Err);
    return EXIT_FAILURE;
  }

  {
    Validator::Validator ValidatorEngine(Conf);
    if (auto Res = ValidatorEngine.validate(*Module); !Res) {
      const auto Err = static_cast<uint32_t>(Res.error());
      spdlog::error("Validate Module failed. Error code: {}", Err);
      return EXIT_FAILURE;
    }
  }

  {
    if (Opt.ConfDumpIR.value()) {
      Conf.getCompilerConfigure().setDumpIR(true);
    }
    if (Opt.ConfInterruptible.value()) {
      Conf.getCompilerConfigure().setInterruptible(true);
    }
    if (Opt.ConfEnableAllStatistics.value()) {
      Conf.getStatisticsConfigure().setInstructionCounting(true);
      Conf.getStatisticsConfigure().setCostMeasuring(true);
      Conf.getStatisticsConfigure().setTimeMeasuring(true);
    } else {
      if (Opt.ConfEnableInstructionCounting.value()) {
        Conf.getStatisticsConfigure().setInstructionCounting(true);
      }
      if (Opt.ConfEnableGasMeasuring.value()) {
        Conf.getStatisticsConfigure().setCostMeasuring(true);
      }
      if (Opt.ConfEnableTimeMeasuring.value()) {
        Conf.getStatisticsConfigure().setTimeMeasuring(true);
      }
    }
    if (Opt.ConfGenericBinary.value()) {
      Conf.getCompilerConfigure().setGenericBinary(true);
    }
    if (OutputPath.extension().u8string() == WASMEDGE_LIB_EXTENSION) {
      Conf.getCompilerConfigure().setOutputFormat(
          CompilerConfigure::OutputFormat::Native);
    }
    LLVM::Compiler Compiler(Conf);
    LLVM::CodeGen CodeGen(Conf);
    if (auto Res = Compiler.compile(*Module); !Res) {
      const auto Err = static_cast<uint32_t>(Res.error());
      spdlog::error("Compilation failed. Error code: {}", Err);
      return EXIT_FAILURE;
    } else if (auto Res2 = CodeGen.codegen(Data, std::move(*Res), OutputPath);
               !Res2) {
      const auto Err = static_cast<uint32_t>(Res2.error());
      spdlog::error("Code Generation failed. Error code: {}", Err);
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
#else
  spdlog::error("Compilation is not supported!");

  return EXIT_FAILURE;
#endif
}

} // namespace Driver
} // namespace WasmEdge
