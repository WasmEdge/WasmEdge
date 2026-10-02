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
#include <system_error>

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
getAbsolutePath(const std::string &Name, std::string_view Kind) noexcept {
  // std::filesystem::absolute throws on an empty path on some standard
  // libraries; reject it here and take the error code for the other failures.
  if (Name.empty()) {
    spdlog::error("No {} provided."sv, Kind);
    return std::nullopt;
  }
  std::error_code Err;
  auto Path = std::filesystem::absolute(u8path(Name), Err);
  if (Err) {
    spdlog::error("Invalid {} path \"{}\": {}"sv, Kind, Name, Err.message());
    return std::nullopt;
  }
  return Path;
}

std::optional<std::filesystem::path>
getInputPath(const struct DriverToolOptions &Opt) noexcept {
  return getAbsolutePath(Opt.SoName.value(), "input wasm file"sv);
}

bool setMemoryPageLimit(const struct DriverToolOptions &Opt,
                        Configure &Conf) noexcept {
  if (Opt.MemLim.value().size() > 0) {
    if (Opt.MemLim.value().back() < 0) {
      spdlog::error("--memory-page-limit value cannot be negative."sv);
      return false;
    }
    Conf.getRuntimeConfigure().setMaxMemoryPage(
        static_cast<uint32_t>(Opt.MemLim.value().back()));
  }
  return true;
}

bool registerLinkedModules(const struct DriverToolOptions &Opt,
                           VM::VM &VM) noexcept {
  for (const auto &ModEntry : Opt.LinkedModules.value()) {
    const auto Pos = ModEntry.find(':');
    if (Pos == std::string::npos || Pos == 0 || Pos + 1 == ModEntry.size()) {
      spdlog::error("Invalid --module format: \"{}\". Expected name:path."sv,
                    ModEntry);
      return false;
    }
    const auto Name = ModEntry.substr(0, Pos);
    const auto Path =
        getAbsolutePath(ModEntry.substr(Pos + 1), "module file"sv);
    if (!Path) {
      return false;
    }
    if (auto Result = VM.registerModule(Name, *Path); !Result) {
      spdlog::error("Failed to register module \"{}\" from: {}"sv, Name,
                    u8string(*Path));
      return false;
    }
  }
  return true;
}

} // namespace Driver
} // namespace WasmEdge
