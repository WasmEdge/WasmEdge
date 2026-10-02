// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "common/configure.h"
#include "common/filesystem.h"
#include "common/spdlog.h"
#include "driver/options.h"
#include "driver/tool.h"
#include "vm/vm.h"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

using namespace std::literals;

namespace WasmEdge {
namespace Driver {

Configure
createProposalConfigure(const struct DriverProposalOptions &Opt) noexcept {
  Configure Conf;
  // WASM standard configuration has the highest priority.
  if (Opt.PropWASM1.value()) {
    Conf.setWASMStandard(Standard::WASM_1);
  }
  if (Opt.PropWASM2.value()) {
    Conf.setWASMStandard(Standard::WASM_2);
  }
  if (Opt.PropWASM3.value()) {
    Conf.setWASMStandard(Standard::WASM_3);
  }

  // Proposals adjustment.
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
  if (Opt.PropSIMD.value()) {
    Conf.removeProposal(Proposal::SIMD);
  }
  if (Opt.PropTailCall.value()) {
    Conf.removeProposal(Proposal::TailCall);
  }
  if (Opt.PropExtendConst.value()) {
    Conf.removeProposal(Proposal::ExtendedConst);
  }
  if (Opt.PropMultiMem.value()) {
    Conf.removeProposal(Proposal::MultiMemories);
  }
  if (Opt.PropRelaxedSIMD.value()) {
    Conf.removeProposal(Proposal::RelaxSIMD);
  }
  if (Opt.PropExceptionHandling.value()) {
    Conf.removeProposal(Proposal::ExceptionHandling);
  }
  if (Opt.PropMemory64.value()) {
    Conf.removeProposal(Proposal::Memory64);
  }

  // Handle the proposal removal which has dependency.
  // The GC proposal depends on the func-ref proposal, and the func-ref proposal
  // depends on the ref-types proposal.
  if (Opt.PropGC.value()) {
    Conf.removeProposal(Proposal::GC);
  }
  if (Opt.PropFunctionReference.value()) {
    // This will automatically not work if the GC proposal not disabled.
    Conf.removeProposal(Proposal::FunctionReferences);
  }
  if (Opt.PropRefTypes.value()) {
    // This will automatically not work if the GC or func-ref proposal not
    // disabled.
    Conf.removeProposal(Proposal::ReferenceTypes);
  }

  if (Opt.PropThreads.value()) {
    Conf.addProposal(Proposal::Threads);
  }
  if (Opt.PropAll.value()) {
    Conf.setWASMStandard(Standard::WASM_3);
    Conf.addProposal(Proposal::Threads);
  }

  return Conf;
}

void setStatisticsConfigure(const struct DriverStatisticsOptions &Opt,
                            Configure &Conf) noexcept {
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
}

void setLoggingLevel(const struct DriverLoggingOptions &Opt) noexcept {
  const std::string &Level =
      Opt.LogLevel.value().empty() ? "info" : Opt.LogLevel.value();
  if (!Log::setLoggingLevelFromString(Level)) {
    spdlog::warn("Invalid log level: {}. Valid values are: off, trace, debug, "
                 "info, warning, error, fatal. Falling back to info level."sv,
                 Level);
    Log::setInfoLoggingLevel();
  }
}

Configure createConfigure(const struct DriverToolOptions &Opt) noexcept {
  setLoggingLevel(Opt);

  Configure Conf = createProposalConfigure(Opt);

  if (Opt.PropComponent.value()) {
    Conf.addProposal(Proposal::Component);
    spdlog::warn("component model is enabled, this is experimental."sv);
  }
  if (Opt.PropAll.value()) {
    spdlog::warn("component model is enabled, this is experimental."sv);
    Conf.addProposal(Proposal::Component);
  }

  for (const auto &Name : Opt.ForbiddenPlugins.value()) {
    Conf.addForbiddenPlugins(Name);
  }

  return Conf;
}

std::optional<std::filesystem::path>
getInputPath(const struct DriverToolOptions &Opt) noexcept {
  // Reject an empty input path here: std::filesystem::absolute("") throws on
  // some standard library implementations (e.g. libstdc++), which would
  // std::terminate() the noexcept tool functions instead of failing
  // gracefully.
  if (Opt.SoName.value().empty()) {
    spdlog::error("No input wasm file provided."sv);
    return std::nullopt;
  }
  return std::filesystem::absolute(u8path(Opt.SoName.value()));
}

void setMemoryPageLimit(const struct DriverToolOptions &Opt,
                        Configure &Conf) noexcept {
  if (Opt.MemLim.value().size() > 0) {
    Conf.getRuntimeConfigure().setMaxMemoryPage(Opt.MemLim.value().back());
  }
}

bool registerLinkedModules(const struct DriverToolOptions &Opt,
                           VM::VM &VM) noexcept {
  for (const auto &ModEntry : Opt.LinkedModules.value()) {
    auto Pos = ModEntry.find(':');
    if (Pos == std::string::npos) {
      spdlog::error("Invalid --module format: \"{}\". Expected name:path."sv,
                    ModEntry);
      return false;
    }
    auto Name = ModEntry.substr(0, Pos);
    auto Path = std::filesystem::absolute(u8path(ModEntry.substr(Pos + 1)));
    if (auto Result = VM.registerModule(Name, Path); !Result) {
      spdlog::error("Failed to register module \"{}\" from: {}"sv, Name,
                    u8string(Path));
      return false;
    }
  }
  return true;
}

} // namespace Driver
} // namespace WasmEdge
