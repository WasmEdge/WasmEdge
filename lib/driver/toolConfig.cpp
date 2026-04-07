// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/configure.h"
#include "common/spdlog.h"
#include "driver/options.h"
#include "driver/tool.h"

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
  if (Opt.PropMemory64.value()) {
    Conf.removeProposal(Proposal::Memory64);
  }
  if (Opt.PropTailCallDeprecated.value()) {
    Conf.addProposal(Proposal::TailCall);
  }
  if (Opt.PropExtendConstDeprecated.value()) {
    Conf.addProposal(Proposal::ExtendedConst);
  }
  if (Opt.PropMultiMemDeprecated.value()) {
    Conf.addProposal(Proposal::MultiMemories);
  }
  if (Opt.PropRelaxedSIMDDeprecated.value()) {
    Conf.addProposal(Proposal::RelaxSIMD);
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
  if (Opt.PropFunctionReferenceDeprecated.value()) {
    Conf.addProposal(Proposal::FunctionReferences);
  }
  if (Opt.PropGCDeprecated.value()) {
    Conf.addProposal(Proposal::GC);
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

Configure createConfigure(const struct DriverToolOptions &Opt) noexcept {
  // Setup logging
  const std::string &Level =
      Opt.LogLevel.value().empty() ? "info" : Opt.LogLevel.value();
  if (!Log::setLoggingLevelFromString(Level)) {
    spdlog::warn("Invalid log level: {}. Valid values are: off, trace, debug, "
                 "info, warning, error, fatal. Falling back to info level."sv,
                 Level);
    Log::setInfoLoggingLevel();
  }

  Configure Conf = createProposalConfigure(Opt);

  if (Opt.PropExceptionHandling.value()) {
    Conf.removeProposal(Proposal::ExceptionHandling);
  }
  if (Opt.PropExceptionHandlingDeprecated.value()) {
    Conf.addProposal(Proposal::ExceptionHandling);
  }
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

} // namespace Driver
} // namespace WasmEdge
