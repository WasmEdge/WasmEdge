// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "common/configure.h"
#include "common/filesystem.h"
#include "common/types.h"
#include "common/version.h"
#include "host/wasi/wasimodule.h"
#include "host/wasmedge_process/processmodule.h"
#include "po/argument_parser.h"
#include "vm/vm.h"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

int main(int Argc, const char *Argv[]) {
  namespace PO = WasmEdge::PO;
  using namespace std::literals;

  std::ios::sync_with_stdio(false);
  WasmEdge::Log::setInfoLoggingLevel();

  PO::Option<std::string> SoName(PO::Description("Wasm or so file"sv),
                                 PO::MetaVar("WASM_OR_SO"sv));
  PO::List<std::string> Args(PO::Description("Execution arguments"sv),
                             PO::MetaVar("ARG"sv));

  PO::Option<PO::Toggle> Reactor(PO::Description(
      "Enable reactor mode. Reactor mode calls `_initialize` if exported."));

  PO::List<std::string> Dir(
      PO::Description(
          "Binding directories into WASI virtual filesystem. Each directories "
          "can specified as --dir `guest_path:host_path`, where `guest_path` "
          "specifies the path that will correspond to `host_path` for calls "
          "like `fopen` in the guest."sv),
      PO::MetaVar("PREOPEN_DIRS"sv));

  PO::List<std::string> Env(
      PO::Description(
          "Environ variables. Each variable can be specified as --env `NAME=VALUE`."sv),
      PO::MetaVar("ENVS"sv));

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
  PO::Option<PO::Toggle> PropThreads(
      PO::Description("Enable Threads proposal"sv));
  PO::Option<PO::Toggle> PropAll(PO::Description("Enable all features"sv));

  PO::Option<PO::Toggle> ConfEnableInstructionCounting(PO::Description(
      "Enable generating code for counting Wasm instructions executed."sv));
  PO::Option<PO::Toggle> ConfEnableGasMeasuring(PO::Description(
      "Enable generating code for counting gas burned during execution."sv));
  PO::Option<PO::Toggle> ConfEnableTimeMeasuring(PO::Description(
      "Enable generating code for counting time during execution."sv));
  PO::Option<PO::Toggle> ConfEnableAllStatistics(PO::Description(
      "Enable generating code for all statistics options include instruction counting, gas measuring, and execution time"sv));

  PO::Option<uint64_t> TimeLim(
      PO::Description(
          "Limitation of maximum time(in milliseconds) for execution, default value is 0 for no limitations"sv),
      PO::MetaVar("TIMEOUT"sv), PO::DefaultValue<uint64_t>(0));

  PO::List<int> GasLim(
      PO::Description(
          "Limitation of execution gas. Upper bound can be specified as --gas-limit `GAS_LIMIT`."sv),
      PO::MetaVar("GAS_LIMIT"sv));

  PO::List<int> MemLim(
      PO::Description(
          "Limitation of pages(as size of 64 KiB) in every memory instance. Upper bound can be specified as --memory-page-limit `PAGE_COUNT`."sv),
      PO::MetaVar("PAGE_COUNT"sv));

  PO::List<std::string> AllowCmd(
      PO::Description(
          "Allow commands called from wasmedge_process host functions. Each command can be specified as --allow-command `COMMAND`."sv),
      PO::MetaVar("COMMANDS"sv));
  PO::Option<PO::Toggle> AllowCmdAll(PO::Description(
      "Allow all commands called from wasmedge_process host functions."sv));

  auto Parser = PO::ArgumentParser();
  if (!Parser.add_option(SoName)
           .add_option(Args)
           .add_option("reactor"sv, Reactor)
           .add_option("dir"sv, Dir)
           .add_option("env"sv, Env)
           .add_option("enable-instruction-count"sv,
                       ConfEnableInstructionCounting)
           .add_option("enable-gas-measuring"sv, ConfEnableGasMeasuring)
           .add_option("enable-time-measuring"sv, ConfEnableTimeMeasuring)
           .add_option("enable-all-statistics"sv, ConfEnableAllStatistics)
           .add_option("disable-import-export-mut-globals"sv, PropMutGlobals)
           .add_option("disable-non-trap-float-to-int"sv, PropNonTrapF2IConvs)
           .add_option("disable-sign-extension-operators"sv, PropSignExtendOps)
           .add_option("disable-multi-value"sv, PropMultiValue)
           .add_option("disable-bulk-memory"sv, PropBulkMemOps)
           .add_option("disable-reference-types"sv, PropRefTypes)
           .add_option("disable-simd"sv, PropSIMD)
           .add_option("enable-multi-memory"sv, PropMultiMem)
           .add_option("enable-threads"sv, PropThreads)
           .add_option("enable-all"sv, PropAll)
           .add_option("time-limit"sv, TimeLim)
           .add_option("gas-limit"sv, GasLim)
           .add_option("memory-page-limit"sv, MemLim)
           .add_option("allow-command"sv, AllowCmd)
           .add_option("allow-command-all"sv, AllowCmdAll)
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
  if (PropThreads.value()) {
    Conf.addProposal(WasmEdge::Proposal::Threads);
  }
  if (PropAll.value()) {
    Conf.addProposal(WasmEdge::Proposal::MultiMemories);
    Conf.addProposal(WasmEdge::Proposal::Threads);
  }

  std::optional<std::chrono::system_clock::time_point> Timeout;
  if (TimeLim.value() > 0) {
    Timeout = std::chrono::system_clock::now() +
              std::chrono::milliseconds(TimeLim.value());
  }
  if (GasLim.value().size() > 0) {
    Conf.getStatisticsConfigure().setCostMeasuring(true);
    Conf.getStatisticsConfigure().setCostLimit(
        static_cast<uint32_t>(GasLim.value().back()));
  }
  if (MemLim.value().size() > 0) {
    Conf.getRuntimeConfigure().setMaxMemoryPage(
        static_cast<uint32_t>(MemLim.value().back()));
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

  Conf.addHostRegistration(WasmEdge::HostRegistration::Wasi);
  Conf.addHostRegistration(WasmEdge::HostRegistration::WasmEdge_Process);
  const auto InputPath = std::filesystem::absolute(SoName.value());
  WasmEdge::VM::VM VM(Conf);

  WasmEdge::Host::WasiModule *WasiMod =
      dynamic_cast<WasmEdge::Host::WasiModule *>(
          VM.getImportModule(WasmEdge::HostRegistration::Wasi));
  WasmEdge::Host::WasmEdgeProcessModule *ProcMod =
      dynamic_cast<WasmEdge::Host::WasmEdgeProcessModule *>(
          VM.getImportModule(WasmEdge::HostRegistration::WasmEdge_Process));

  if (AllowCmdAll.value()) {
    ProcMod->getEnv().AllowedAll = true;
  }
  for (auto &Str : AllowCmd.value()) {
    ProcMod->getEnv().AllowedCmd.insert(Str);
  }

  WasiMod->getEnv().init(
      Dir.value(),
      InputPath.filename()
          .replace_extension(std::filesystem::u8path("wasm"sv))
          .u8string(),
      Args.value(), Env.value());

  if (!Reactor.value()) {
    // command mode
    auto AsyncResult = VM.asyncRunWasmFile(InputPath.u8string(), "_start");
    if (Timeout.has_value()) {
      if (!AsyncResult.waitUntil(*Timeout)) {
        AsyncResult.cancel();
      }
    }
    if (auto Result = AsyncResult.get();
        Result || Result.error() == WasmEdge::ErrCode::Terminated) {
      return static_cast<int>(WasiMod->getEnv().getExitCode());
    } else {
      return EXIT_FAILURE;
    }
  } else {
    // reactor mode
    if (Args.value().empty()) {
      std::cerr
          << "A function name is required when reactor mode is enabled.\n";
      return EXIT_FAILURE;
    }
    const auto &FuncName = Args.value().front();
    if (auto Result = VM.loadWasm(InputPath.u8string()); !Result) {
      return EXIT_FAILURE;
    }
    if (auto Result = VM.validate(); !Result) {
      return EXIT_FAILURE;
    }
    if (auto Result = VM.instantiate(); !Result) {
      return EXIT_FAILURE;
    }

    using namespace std::literals::string_literals;
    const auto InitFunc = "_initialize"s;

    bool HasInit = false;
    WasmEdge::AST::FunctionType FuncType;

    for (const auto &Func : VM.getFunctionList()) {
      if (Func.first == InitFunc) {
        HasInit = true;
      } else if (Func.first == FuncName) {
        FuncType = Func.second;
      }
    }

    if (HasInit) {
      auto AsyncResult = VM.asyncExecute(InitFunc);
      if (Timeout.has_value()) {
        if (!AsyncResult.waitUntil(*Timeout)) {
          AsyncResult.cancel();
        }
      }
      if (auto Result = AsyncResult.get(); WasmEdge::unlikely(!Result)) {
        return EXIT_FAILURE;
      }
    }

    std::vector<WasmEdge::ValVariant> FuncArgs;
    std::vector<WasmEdge::ValType> FuncArgTypes;
    for (size_t I = 0;
         I < FuncType.getParamTypes().size() && I + 1 < Args.value().size();
         ++I) {
      switch (FuncType.getParamTypes()[I]) {
      case WasmEdge::ValType::I32: {
        const uint32_t Value =
            static_cast<uint32_t>(std::stol(Args.value()[I + 1]));
        FuncArgs.emplace_back(Value);
        FuncArgTypes.emplace_back(WasmEdge::ValType::I32);
        break;
      }
      case WasmEdge::ValType::I64: {
        const uint64_t Value =
            static_cast<uint64_t>(std::stoll(Args.value()[I + 1]));
        FuncArgs.emplace_back(Value);
        FuncArgTypes.emplace_back(WasmEdge::ValType::I64);
        break;
      }
      case WasmEdge::ValType::F32: {
        const float Value = std::stof(Args.value()[I + 1]);
        FuncArgs.emplace_back(Value);
        FuncArgTypes.emplace_back(WasmEdge::ValType::F32);
        break;
      }
      case WasmEdge::ValType::F64: {
        const double Value = std::stod(Args.value()[I + 1]);
        FuncArgs.emplace_back(Value);
        FuncArgTypes.emplace_back(WasmEdge::ValType::F64);
        break;
      }
      /// TODO: FuncRef and ExternRef
      default:
        break;
      }
    }
    if (FuncType.getParamTypes().size() + 1 < Args.value().size()) {
      for (size_t I = FuncType.getParamTypes().size() + 1;
           I < Args.value().size(); ++I) {
        const uint64_t Value =
            static_cast<uint64_t>(std::stoll(Args.value()[I]));
        FuncArgs.emplace_back(Value);
        FuncArgTypes.emplace_back(WasmEdge::ValType::F64);
      }
    }

    auto AsyncResult = VM.asyncExecute(FuncName, FuncArgs, FuncArgTypes);
    if (Timeout.has_value()) {
      if (!AsyncResult.waitUntil(*Timeout)) {
        AsyncResult.cancel();
      }
    }
    if (auto Result = AsyncResult.get()) {
      /// Print results.
      for (size_t I = 0; I < Result->size(); ++I) {
        switch ((*Result)[I].second) {
        case WasmEdge::ValType::I32:
          std::cout << (*Result)[I].first.get<uint32_t>() << '\n';
          break;
        case WasmEdge::ValType::I64:
          std::cout << (*Result)[I].first.get<uint64_t>() << '\n';
          break;
        case WasmEdge::ValType::F32:
          std::cout << (*Result)[I].first.get<float>() << '\n';
          break;
        case WasmEdge::ValType::F64:
          std::cout << (*Result)[I].first.get<double>() << '\n';
          break;
        /// TODO: FuncRef and ExternRef
        default:
          break;
        }
      }
      return EXIT_SUCCESS;
    } else {
      return EXIT_FAILURE;
    }
  }
}
