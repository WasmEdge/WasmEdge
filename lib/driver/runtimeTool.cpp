// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "common/configure.h"
#include "common/filesystem.h"
#include "common/log.h"
#include "common/types.h"
#include "common/version.h"
#include "driver/tool.h"
#include "host/wasi/wasimodule.h"
#include "vm/vm.h"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace WasmEdge {
namespace Driver {

int Tool(int Argc, const char *Argv[]) noexcept {
  using namespace std::literals;

  std::ios::sync_with_stdio(false);
  Log::setInfoLoggingLevel();

  
  auto Parser = PO::ArgumentParser();
  struct DriverToolOptions opt;
  
  opt.add_option(Parser);


  if (!Parser.parse(stdout, Argc, Argv)) {
    return EXIT_FAILURE;
  }
  if (Parser.isVersion()) {
    std::cout << Argv[0] << " version "sv << kVersionString << '\n';
    return EXIT_SUCCESS;
  }

  Configure Conf;
  if (opt.PropMutGlobals.value()) {
    Conf.removeProposal(Proposal::ImportExportMutGlobals);
  }
  if (opt.PropNonTrapF2IConvs.value()) {
    Conf.removeProposal(Proposal::NonTrapFloatToIntConversions);
  }
  if (opt.PropSignExtendOps.value()) {
    Conf.removeProposal(Proposal::SignExtensionOperators);
  }
  if (opt.PropMultiValue.value()) {
    Conf.removeProposal(Proposal::MultiValue);
  }
  if (opt.PropBulkMemOps.value()) {
    Conf.removeProposal(Proposal::BulkMemoryOperations);
  }
  if (opt.PropRefTypes.value()) {
    Conf.removeProposal(Proposal::ReferenceTypes);
  }
  if (opt.PropSIMD.value()) {
    Conf.removeProposal(Proposal::SIMD);
  }
  if (opt.PropMultiMem.value()) {
    Conf.addProposal(Proposal::MultiMemories);
  }
  if (opt.PropTailCall.value()) {
    Conf.addProposal(Proposal::TailCall);
  }
  if (opt.PropExtendConst.value()) {
    Conf.addProposal(Proposal::ExtendedConst);
  }
  if (opt.PropThreads.value()) {
    Conf.addProposal(Proposal::Threads);
  }
  if (opt.PropAll.value()) {
    Conf.addProposal(Proposal::MultiMemories);
    Conf.addProposal(Proposal::TailCall);
    Conf.addProposal(Proposal::ExtendedConst);
    Conf.addProposal(Proposal::Threads);
  }

  std::optional<std::chrono::system_clock::time_point> Timeout;
  if (opt.TimeLim.value() > 0) {
    Timeout = std::chrono::system_clock::now() +
              std::chrono::milliseconds(opt.TimeLim.value());
  }
  if (opt.GasLim.value().size() > 0) {
    Conf.getStatisticsConfigure().setCostMeasuring(true);
    Conf.getStatisticsConfigure().setCostLimit(
        static_cast<uint32_t>(opt.GasLim.value().back()));
  }
  if (opt.MemLim.value().size() > 0) {
    Conf.getRuntimeConfigure().setMaxMemoryPage(
        static_cast<uint32_t>(opt.MemLim.value().back()));
  }
  if (opt.ConfEnableAllStatistics.value()) {
    Conf.getStatisticsConfigure().setInstructionCounting(true);
    Conf.getStatisticsConfigure().setCostMeasuring(true);
    Conf.getStatisticsConfigure().setTimeMeasuring(true);
  } else {
    if (opt.ConfEnableInstructionCounting.value()) {
      Conf.getStatisticsConfigure().setInstructionCounting(true);
    }
    if (opt.ConfEnableGasMeasuring.value()) {
      Conf.getStatisticsConfigure().setCostMeasuring(true);
    }
    if (opt.ConfEnableTimeMeasuring.value()) {
      Conf.getStatisticsConfigure().setTimeMeasuring(true);
    }
  }
  if (opt.ConfForceInterpreter.value()) {
    Conf.getRuntimeConfigure().setForceInterpreter(true);
  }

  for (const auto &Name : opt.ForbiddenPlugins.value()) {
    Conf.addForbiddenPlugins(Name);
  }

  Conf.addHostRegistration(HostRegistration::Wasi);
  const auto InputPath = std::filesystem::absolute(opt.SoName.value());
  VM::VM VM(Conf);

  Host::WasiModule *WasiMod = dynamic_cast<Host::WasiModule *>(
      VM.getImportModule(HostRegistration::Wasi));

  if (auto Result = VM.loadWasm(InputPath.u8string()); !Result) {
    return EXIT_FAILURE;
  }
  if (auto Result = VM.validate(); !Result) {
    return EXIT_FAILURE;
  }
  if (auto Result = VM.instantiate(); !Result) {
    return EXIT_FAILURE;
  }

  auto HasValidCommandModStartFunc = [&]() {
    bool HasStart = false;
    bool Valid = false;

    auto Functions = VM.getFunctionList();
    for (auto &[FuncName, Type] : Functions) {
      if (FuncName == "_start") {
        HasStart = true;
        if (Type.getReturnTypes().size() == 0 &&
            Type.getParamTypes().size() == 0) {
          Valid = true;
          break;
        }
      }
    }

    // if HasStart but not Valid, insert _start to enter reactor mode
    if (HasStart && !Valid) {
      opt.Args.value().insert(opt.Args.value().begin(), "_start");
    }

    return HasStart && Valid;
  };

  bool EnterCommandMode = !opt.Reactor.value() && HasValidCommandModStartFunc();

  WasiMod->getEnv().init(
      opt.Dir.value(),
      InputPath.filename()
          .replace_extension(std::filesystem::u8path("wasm"sv))
          .u8string(),
      opt.Args.value(), opt.Env.value());

  if (EnterCommandMode) {
    // command mode
    auto AsyncResult = VM.asyncExecute("_start"sv);
    if (Timeout.has_value()) {
      if (!AsyncResult.waitUntil(*Timeout)) {
        AsyncResult.cancel();
      }
    }
    if (auto Result = AsyncResult.get();
        Result || Result.error() == ErrCode::Value::Terminated) {
      return static_cast<int>(WasiMod->getEnv().getExitCode());
    } else {
      // It indicates that the execution of wasm has been aborted
      return 128 + SIGABRT;
    }
  } else {
    // reactor mode
    if (opt.Args.value().empty()) {
      std::cerr
          << "A function name is required when reactor mode is enabled.\n";
      return EXIT_FAILURE;
    }
    const auto &FuncName = opt.Args.value().front();

    using namespace std::literals::string_literals;
    const auto InitFunc = "_initialize"s;

    bool HasInit = false;
    AST::FunctionType FuncType;

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
      if (auto Result = AsyncResult.get(); unlikely(!Result)) {
        // It indicates that the execution of wasm has been aborted
        return 128 + SIGABRT;
      }
    }

    std::vector<ValVariant> FuncArgs;
    std::vector<ValType> FuncArgTypes;
    for (size_t I = 0;
         I < FuncType.getParamTypes().size() && I + 1 < opt.Args.value().size();
         ++I) {
      switch (FuncType.getParamTypes()[I]) {
      case ValType::I32: {
        const uint32_t Value =
            static_cast<uint32_t>(std::stol(opt.Args.value()[I + 1]));
        FuncArgs.emplace_back(Value);
        FuncArgTypes.emplace_back(ValType::I32);
        break;
      }
      case ValType::I64: {
        const uint64_t Value =
            static_cast<uint64_t>(std::stoll(opt.Args.value()[I + 1]));
        FuncArgs.emplace_back(Value);
        FuncArgTypes.emplace_back(ValType::I64);
        break;
      }
      case ValType::F32: {
        const float Value = std::stof(opt.Args.value()[I + 1]);
        FuncArgs.emplace_back(Value);
        FuncArgTypes.emplace_back(ValType::F32);
        break;
      }
      case ValType::F64: {
        const double Value = std::stod(opt.Args.value()[I + 1]);
        FuncArgs.emplace_back(Value);
        FuncArgTypes.emplace_back(ValType::F64);
        break;
      }
      /// TODO: FuncRef and ExternRef
      default:
        break;
      }
    }
    if (FuncType.getParamTypes().size() + 1 < opt.Args.value().size()) {
      for (size_t I = FuncType.getParamTypes().size() + 1;
           I < opt.Args.value().size(); ++I) {
        const uint64_t Value =
            static_cast<uint64_t>(std::stoll(opt.Args.value()[I]));
        FuncArgs.emplace_back(Value);
        FuncArgTypes.emplace_back(ValType::I64);
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
        case ValType::I32:
          std::cout << (*Result)[I].first.get<uint32_t>() << '\n';
          break;
        case ValType::I64:
          std::cout << (*Result)[I].first.get<uint64_t>() << '\n';
          break;
        case ValType::F32:
          std::cout << (*Result)[I].first.get<float>() << '\n';
          break;
        case ValType::F64:
          std::cout << (*Result)[I].first.get<double>() << '\n';
          break;
        case ValType::V128:
          std::cout << (*Result)[I].first.get<uint128_t>() << '\n';
          break;
        /// TODO: FuncRef and ExternRef
        default:
          break;
        }
      }
      return EXIT_SUCCESS;
    } else {
      // It indicates that the execution of wasm has been aborted
      return 128 + SIGABRT;
    }
  }
}

} // namespace Driver
} // namespace WasmEdge
