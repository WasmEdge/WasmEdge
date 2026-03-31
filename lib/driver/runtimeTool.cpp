// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/configure.h"
#include "common/filesystem.h"
#include "common/spdlog.h"
#include "common/types.h"
#include "common/version.h"
#include "driver/tool.h"
#include "host/wasi/wasimodule.h"
#include "vm/vm.h"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using namespace std::literals;

namespace WasmEdge {
namespace Driver {

static int
ToolOnModule(WasmEdge::VM::VM &VM, const std::string &FuncName,
             std::optional<std::chrono::system_clock::time_point> Timeout,
             struct DriverToolOptions &Opt,
             const AST::FunctionType &FuncType) noexcept {
  std::vector<ValVariant> FuncArgs;
  std::vector<ValType> FuncArgTypes;

  for (size_t I = 0;
       I < FuncType.getParamTypes().size() && I + 1 < Opt.Args.value().size();
       ++I) {
    const auto TCode = FuncType.getParamTypes()[I].getCode();
    switch (TCode) {
    case TypeCode::I32: {
      const int32_t Value =
          static_cast<int32_t>(std::stol(Opt.Args.value()[I + 1]));
      FuncArgs.emplace_back(Value);
      FuncArgTypes.emplace_back(TCode);
      break;
    }
    case TypeCode::I64: {
      const int64_t Value =
          static_cast<int64_t>(std::stoll(Opt.Args.value()[I + 1]));
      FuncArgs.emplace_back(Value);
      FuncArgTypes.emplace_back(TCode);
      break;
    }
    case TypeCode::F32: {
      const float Value = std::stof(Opt.Args.value()[I + 1]);
      FuncArgs.emplace_back(Value);
      FuncArgTypes.emplace_back(TCode);
      break;
    }
    case TypeCode::F64: {
      const double Value = std::stod(Opt.Args.value()[I + 1]);
      FuncArgs.emplace_back(Value);
      FuncArgTypes.emplace_back(TCode);
      break;
    }
    // TODO: FuncRef and ExternRef
    default:
      break;
    }
  }
  if (FuncType.getParamTypes().size() + 1 < Opt.Args.value().size()) {
    for (size_t I = FuncType.getParamTypes().size() + 1;
         I < Opt.Args.value().size(); ++I) {
      const uint64_t Value =
          static_cast<uint64_t>(std::stoll(Opt.Args.value()[I]));
      FuncArgs.emplace_back(Value);
      FuncArgTypes.emplace_back(TypeCode::I64);
    }
  }

  auto AsyncResult = VM.asyncExecute(FuncName, FuncArgs, FuncArgTypes);
  if (Timeout.has_value()) {
    if (!AsyncResult.waitUntil(*Timeout)) {
      AsyncResult.cancel();
    }
  }
  if (auto Result = AsyncResult.get()) {
    // Print results.
    for (size_t I = 0; I < Result->size(); ++I) {
      switch ((*Result)[I].second.getCode()) {
      case TypeCode::I32:
        fmt::print("{}\n"sv, (*Result)[I].first.get<int32_t>());
        break;
      case TypeCode::I64:
        fmt::print("{}\n"sv, (*Result)[I].first.get<int64_t>());
        break;
      case TypeCode::F32:
        fmt::print("{}\n"sv, (*Result)[I].first.get<float>());
        break;
      case TypeCode::F64:
        fmt::print("{}\n"sv, (*Result)[I].first.get<double>());
        break;
      case TypeCode::V128:
        fmt::print("{}\n"sv, uint128((*Result)[I].first.get<uint128_t>()));
        break;
      case TypeCode::Ref: {
        if ((*Result)[I].second.isFuncRefType()) {
          fmt::print("<funcref>\n"sv);
        } else if ((*Result)[I].second.isExternRefType()) {
          fmt::print("<externref>\n"sv);
        } else {
          fmt::print("<anyref>\n"sv);
        }
        break;
      }
      case TypeCode::RefNull: {
        if ((*Result)[I].second.isFuncRefType()) {
          fmt::print("<null funcref>\n"sv);
        } else if ((*Result)[I].second.isExternRefType()) {
          fmt::print("<null externref>\n"sv);
        } else {
          fmt::print("<null anyref>\n"sv);
        }
        break;
      }
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

static int
ToolOnComponent(WasmEdge::VM::VM &VM, const std::string &FuncName,
                std::optional<std::chrono::system_clock::time_point> Timeout,
                struct DriverToolOptions &Opt,
                const AST::Component::FuncType &FuncType) noexcept {
  std::vector<ComponentValVariant> FuncArgs;
  std::vector<ComponentValType> FuncArgTypes;

  for (size_t I = 0;
       I < FuncType.getParamList().size() && I + 1 < Opt.Args.value().size();
       ++I) {
    const auto TCode = FuncType.getParamList()[I].getValType().getCode();
    switch (TCode) {
    case ComponentTypeCode::S32:
    case ComponentTypeCode::U32: {
      const uint32_t Value =
          static_cast<uint32_t>(std::stol(Opt.Args.value()[I + 1]));
      FuncArgs.emplace_back(Value);
      FuncArgTypes.emplace_back(TCode);
      break;
    }
    case ComponentTypeCode::S64:
    case ComponentTypeCode::U64: {
      const uint64_t Value =
          static_cast<uint64_t>(std::stoll(Opt.Args.value()[I + 1]));
      FuncArgs.emplace_back(Value);
      FuncArgTypes.emplace_back(TCode);
      break;
    }
    case ComponentTypeCode::F32: {
      const float Value = std::stof(Opt.Args.value()[I + 1]);
      FuncArgs.emplace_back(Value);
      FuncArgTypes.emplace_back(TCode);
      break;
    }
    case ComponentTypeCode::F64: {
      const double Value = std::stod(Opt.Args.value()[I + 1]);
      FuncArgs.emplace_back(Value);
      FuncArgTypes.emplace_back(TCode);
      break;
    }
    case ComponentTypeCode::String: {
      const std::string Value = Opt.Args.value()[I + 1];
      FuncArgs.emplace_back(Value);
      FuncArgTypes.emplace_back(TCode);
      break;
    }
    // TODO: COMPONENT - other types.
    default:
      break;
    }
  }
  if (FuncType.getParamList().size() + 1 < Opt.Args.value().size()) {
    for (size_t I = FuncType.getParamList().size() + 1;
         I < Opt.Args.value().size(); ++I) {
      const uint64_t Value =
          static_cast<uint64_t>(std::stoll(Opt.Args.value()[I]));
      FuncArgs.emplace_back(Value);
      FuncArgTypes.emplace_back(ComponentTypeCode::U64);
    }
  }

  auto AsyncResult = VM.asyncExecuteComponent(FuncName, FuncArgs, FuncArgTypes);
  if (Timeout.has_value()) {
    if (!AsyncResult.waitUntil(*Timeout)) {
      AsyncResult.cancel();
    }
  }
  if (auto Result = AsyncResult.get()) {
    // Print results.
    for (auto &&Val : *Result) {
      switch (Val.second.getCode()) {
      case ComponentTypeCode::S32:
        fmt::print("{}\n"sv, std::get<ValVariant>(Val.first).get<int32_t>());
        break;
      case ComponentTypeCode::U32:
        fmt::print("{}\n"sv, std::get<ValVariant>(Val.first).get<uint32_t>());
        break;
      case ComponentTypeCode::S64:
        fmt::print("{}\n"sv, std::get<ValVariant>(Val.first).get<int64_t>());
        break;
      case ComponentTypeCode::U64:
        fmt::print("{}\n"sv, std::get<ValVariant>(Val.first).get<uint64_t>());
        break;
      case ComponentTypeCode::F32:
        fmt::print("{}\n"sv, std::get<ValVariant>(Val.first).get<float>());
        break;
      case ComponentTypeCode::F64:
        fmt::print("{}\n"sv, std::get<ValVariant>(Val.first).get<double>());
        break;
      case ComponentTypeCode::String:
        fmt::print("{}\n"sv, std::get<std::string>(Val.first));
        break;
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

int Tool(struct DriverToolOptions &Opt) noexcept {
  std::ios::sync_with_stdio(false);

  Configure Conf = createConfigure(Opt);

  std::optional<std::chrono::system_clock::time_point> Timeout;
  if (Opt.TimeLim.value() > 0) {
    Timeout = std::chrono::system_clock::now() +
              std::chrono::milliseconds(Opt.TimeLim.value());
  }
  if (Opt.GasLim.value().size() > 0) {
    Conf.getStatisticsConfigure().setCostMeasuring(true);
    Conf.getStatisticsConfigure().setCostLimit(
        static_cast<uint32_t>(Opt.GasLim.value().back()));
  }
  if (Opt.MemLim.value().size() > 0) {
    Conf.getRuntimeConfigure().setMaxMemoryPage(
        static_cast<uint32_t>(Opt.MemLim.value().back()));
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
  if (Opt.ConfEnableJIT.value()) {
    Conf.getRuntimeConfigure().setEnableJIT(true);
    Conf.getCompilerConfigure().setOptimizationLevel(
        WasmEdge::CompilerConfigure::OptimizationLevel::O1);
  }
  if (Opt.ConfEnableCoredump.value()) {
    Conf.getRuntimeConfigure().setEnableCoredump(true);
  }
  if (Opt.ConfCoredumpWasmgdb.value()) {
    Conf.getRuntimeConfigure().setCoredumpWasmgdb(true);
  }
  if (Opt.ConfForceInterpreter.value()) {
    Conf.getRuntimeConfigure().setForceInterpreter(true);
  }
  if (Opt.ConfAFUNIX.value()) {
    Conf.getRuntimeConfigure().setAllowAFUNIX(true);
  }

  Conf.addHostRegistration(HostRegistration::Wasi);
  const auto InputPath =
      std::filesystem::absolute(std::filesystem::u8path(Opt.SoName.value()));

  // Create VM and get WASI module instance.
  VM::VM VM(Conf);
  Host::WasiModule *WasiMod = dynamic_cast<Host::WasiModule *>(
      VM.getImportModule(HostRegistration::Wasi));

  for (const auto &ModEntry : Opt.LinkedModules.value()) {
    auto Pos = ModEntry.find(':');
    if (Pos == std::string::npos) {
      spdlog::error("Invalid --module format: \"{}\". Expected name:path."sv,
                    ModEntry);
      return EXIT_FAILURE;
    }
    auto Name = ModEntry.substr(0, Pos);
    auto Path = std::filesystem::absolute(
        std::filesystem::u8path(ModEntry.substr(Pos + 1)));
    if (auto Result = VM.registerModule(Name, Path); !Result) {
      spdlog::error("Failed to register module \"{}\" from: {}"sv, Name,
                    Path.u8string());
      return EXIT_FAILURE;
    }
  }

  // Load, validate, and instantiate WASM or Component.
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
      Opt.Args.value().insert(Opt.Args.value().begin(), "_start");
    }

    return HasStart && Valid;
  };

  // TODO: COMPONENT - does component start function named as "_start"?
  bool EnterCommandMode = !Opt.Reactor.value() && HasValidCommandModStartFunc();

  // Initialize WASI module.
  WasiMod->init(Opt.Dir.value(),
                InputPath.filename()
                    .replace_extension(std::filesystem::u8path("wasm"sv))
                    .u8string(),
                Opt.Args.value(), Opt.Env.value());

  if (EnterCommandMode) {
    // command mode

    // TODO: COMPONENT - currently not supported.
    auto AsyncResult = VM.asyncExecute("_start"sv);
    if (Timeout.has_value()) {
      if (!AsyncResult.waitUntil(*Timeout)) {
        AsyncResult.cancel();
      }
    }
    if (auto Result = AsyncResult.get();
        Result || Result.error() == ErrCode::Value::Terminated) {
      return static_cast<int>(WasiMod->getExitCode());
    } else {
      // It indicates that the execution of wasm has been aborted
      return 128 + SIGABRT;
    }
  } else {
    // reactor mode

    // Get the function name to invoke.
    if (Opt.Args.value().empty()) {
      fmt::print(
          stderr,
          "A function name is required when reactor mode is enabled.\n"sv);
      return EXIT_FAILURE;
    }
    const auto &FuncName = Opt.Args.value().front();

    if (VM.holdsModule()) {
      // WASM case.

      // Check the exported function name and function type first.
      const auto InitFunc = "_initialize"s;
      bool HasInit = false;
      const AST::FunctionType *FuncType = nullptr;
      for (const auto &Func : VM.getFunctionList()) {
        if (Func.first == InitFunc) {
          // Found the init function.
          HasInit = true;
        }
        if (Func.first == FuncName) {
          // Found the function to invoke.
          FuncType = &Func.second;
        }
      }

      if (FuncType == nullptr) {
        fmt::print(stderr,
                   "Function \"{}\" not found in the module export list.\n"sv,
                   FuncName);
        return EXIT_FAILURE;
      }

      // If found initialize function and it's not being called explicitly,
      // invoke it first.
      if (HasInit && FuncName != InitFunc) {
        auto AsyncResult = VM.asyncExecute(InitFunc);
        if (Timeout.has_value()) {
          if (!AsyncResult.waitUntil(*Timeout)) {
            AsyncResult.cancel();
          }
        }
        if (auto Result = AsyncResult.get(); unlikely(!Result)) {
          // It indicates that the execution of wasm has been aborted.
          return 128 + SIGABRT;
        }
      }
      return ToolOnModule(VM, FuncName, Timeout, Opt, *FuncType);
    } else if (VM.holdsComponent()) {
      // Component case.

      // Check the exported function name and function type first.
      const AST::Component::FuncType *FuncType = nullptr;
      for (const auto &Func : VM.getComponentFunctionList()) {
        if (Func.first == FuncName) {
          // Found the function to invoke.
          FuncType = &Func.second;
        }
      }
      if (FuncType == nullptr) {
        fmt::print(
            stderr,
            "Function \"{}\" not found in the component export list.\n"sv,
            FuncName);
        return EXIT_FAILURE;
      }
      return ToolOnComponent(VM, FuncName, Timeout, Opt, *FuncType);
    } else {
      // which means VM has neither instantiated module nor instantiated
      // component
      return 128 + SIGABRT;
    }
  }
}

} // namespace Driver
} // namespace WasmEdge
